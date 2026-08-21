/**
 * XaoS Community Server — SQLite database layer
 *
 * Schema:
 *   users          — Teachers and students
 *   organizations  — Currently used for flat Rooms/Groups (designed for future hierarchical grouping: class → school → district → country)
 *   org_members    — Links users to organizations
 *   fractals       — Uploaded .xpf positions with metadata
 */

const Database = require("better-sqlite3");
const path = require("path");
const fs = require("fs");

// Database lives alongside the server code
const DB_PATH = path.join(__dirname, "community.db");

let db;

function init() {
  db = new Database(DB_PATH);

  // Enable WAL mode for better concurrent read performance
  db.pragma("journal_mode = WAL");

  db.exec(`
    CREATE TABLE IF NOT EXISTS users (
      id            INTEGER PRIMARY KEY AUTOINCREMENT,
      email         TEXT UNIQUE,
      password_hash TEXT,
      display_name  TEXT    NOT NULL,
      role          TEXT    DEFAULT 'student',
      created_at    DATETIME DEFAULT CURRENT_TIMESTAMP
    );

    -- Replaces the old 'groups' table with a self-referencing hierarchy.
    -- type: 'class', 'school', 'district', 'state', 'country'
    -- parent_id: NULL for top-level orgs, otherwise points to the parent org
    CREATE TABLE IF NOT EXISTS organizations (
      id            INTEGER PRIMARY KEY AUTOINCREMENT,
      name          TEXT    NOT NULL,
      type          TEXT    NOT NULL DEFAULT 'class',
      parent_id     INTEGER,
      created_by    INTEGER,
      invite_code   TEXT    UNIQUE,
      created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY(parent_id) REFERENCES organizations(id),
      FOREIGN KEY(created_by) REFERENCES users(id)
    );

    CREATE TABLE IF NOT EXISTS org_members (
      org_id        INTEGER,
      user_id       INTEGER,
      joined_at     DATETIME DEFAULT CURRENT_TIMESTAMP,
      PRIMARY KEY (org_id, user_id),
      FOREIGN KEY(org_id) REFERENCES organizations(id),
      FOREIGN KEY(user_id) REFERENCES users(id)
    );

    CREATE TABLE IF NOT EXISTS fractals (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      title       TEXT    NOT NULL,
      author      TEXT    DEFAULT 'Anonymous',
      user_id     INTEGER,
      group_id    INTEGER,
      formula     TEXT,
      iterations  INTEGER,
      zoom_level  TEXT,
      xpf_data    TEXT    NOT NULL,
      thumbnail   TEXT,
      downloads   INTEGER DEFAULT 0,
      likes       INTEGER DEFAULT 0,
      created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY(user_id) REFERENCES users(id),
      FOREIGN KEY(group_id) REFERENCES organizations(id)
    );

    CREATE INDEX IF NOT EXISTS idx_fractals_created
      ON fractals(created_at DESC);
    CREATE INDEX IF NOT EXISTS idx_fractals_downloads
      ON fractals(downloads DESC);
    CREATE INDEX IF NOT EXISTS idx_fractals_group
      ON fractals(group_id);
    CREATE INDEX IF NOT EXISTS idx_org_type
      ON organizations(type);
    CREATE INDEX IF NOT EXISTS idx_org_parent
      ON organizations(parent_id);
  `);

  // Migrate: if old 'groups' table exists, copy data to organizations
  try {
    const oldGroups = db.prepare("SELECT * FROM groups").all();
    if (oldGroups.length > 0) {
      const insert = db.prepare(`
        INSERT OR IGNORE INTO organizations (id, name, type, parent_id, created_by, invite_code, created_at)
        VALUES (@id, @name, 'class', NULL, @teacher_id, @invite_code, @created_at)
      `);
      const insertMember = db.prepare(`
        INSERT OR IGNORE INTO org_members (org_id, user_id, joined_at)
        SELECT @org_id, user_id, joined_at FROM group_members WHERE group_id = @org_id
      `);
      const migrate = db.transaction(() => {
        for (const g of oldGroups) {
          insert.run(g);
          insertMember.run({ org_id: g.id });
        }
      });
      migrate();
      console.log(`  Migrated ${oldGroups.length} groups to organizations table`);
    }
  } catch (e) {
    // 'groups' table doesn't exist — fresh install, nothing to migrate
  }

  // Ensure uploads directory exists
  const uploadsDir = path.join(__dirname, "uploads");
  if (!fs.existsSync(uploadsDir)) {
    fs.mkdirSync(uploadsDir, { recursive: true });
  }

  return db;
}

// ─── Fractal Functions ───────────────────────────────────────

/**
 * Insert a new fractal and return its id.
 */
function insertFractal({
  title,
  author,
  userId,
  groupId,
  formula,
  iterations,
  zoomLevel,
  xpfData,
  thumbnail,
}) {
  const stmt = db.prepare(`
    INSERT INTO fractals (title, author, user_id, group_id, formula, iterations, zoom_level, xpf_data, thumbnail)
    VALUES (@title, @author, @userId, @groupId, @formula, @iterations, @zoomLevel, @xpfData, @thumbnail)
  `);

  const result = stmt.run({
    title,
    author: author || "Anonymous",
    userId: userId || null,
    groupId: groupId || null,
    formula: formula || null,
    iterations: iterations || null,
    zoomLevel: zoomLevel || null,
    xpfData,
    thumbnail: thumbnail || null,
  });

  return result.lastInsertRowid;
}

/**
 * Fetch a paginated list of fractals.
 */
function getFractals(page = 1, limit = 20, sort = "recent", groupId = null) {
  const offset = (page - 1) * limit;
  const orderBy = sort === "popular" ? "downloads DESC, likes DESC, created_at DESC" : "created_at DESC";

  let totalStr = "SELECT COUNT(*) as total FROM fractals WHERE group_id IS NULL";
  let itemsStr = `SELECT id, title, author, user_id, group_id, formula, iterations, zoom_level, thumbnail, downloads, likes, created_at FROM fractals WHERE group_id IS NULL ORDER BY ${orderBy} LIMIT @limit OFFSET @offset`;
  let params = { limit, offset };

  if (groupId !== null) {
    totalStr = "SELECT COUNT(*) as total FROM fractals WHERE group_id = @groupId";
    itemsStr = `SELECT id, title, author, user_id, group_id, formula, iterations, zoom_level, thumbnail, downloads, likes, created_at FROM fractals WHERE group_id = @groupId ORDER BY ${orderBy} LIMIT @limit OFFSET @offset`;
    params.groupId = groupId;
  }

  const countRow = db.prepare(totalStr).get(groupId !== null ? { groupId } : {});
  const total = countRow.total;
  const totalPages = Math.ceil(total / limit);

  const items = db.prepare(itemsStr).all(params);

  return { items, total, totalPages, page };
}

/**
 * Fetch a single fractal by id (without xpf_data for metadata-only queries).
 */
function getFractalById(id) {
  return db
    .prepare(
      `
    SELECT id, title, author, user_id, group_id, formula, iterations, zoom_level,
           thumbnail, downloads, likes, created_at
    FROM fractals WHERE id = ?
  `
    )
    .get(id);
}

/**
 * Fetch the XPF data for a fractal and increment download counter.
 */
function getXpfData(id) {
  const row = db
    .prepare("SELECT xpf_data FROM fractals WHERE id = ?")
    .get(id);

  if (row) {
    db.prepare("UPDATE fractals SET downloads = downloads + 1 WHERE id = ?").run(
      id
    );
  }

  return row ? row.xpf_data : null;
}

function incrementLikes(id) {
  db.prepare("UPDATE fractals SET likes = likes + 1 WHERE id = ?").run(id);
}

function decrementLikes(id) {
  db.prepare("UPDATE fractals SET likes = MAX(likes - 1, 0) WHERE id = ?").run(id);
}

// ─── User Functions ──────────────────────────────────────────

function createUser({ email, passwordHash, displayName, role }) {
  const stmt = db.prepare(`INSERT INTO users (email, password_hash, display_name, role) VALUES (@email, @passwordHash, @displayName, @role)`);
  return stmt.run({ email: email || null, passwordHash: passwordHash || null, displayName, role: role || 'student' }).lastInsertRowid;
}

function getUserByEmail(email) {
  return db.prepare("SELECT * FROM users WHERE email = ?").get(email);
}

// ─── Organization Functions (replaces old Group functions) ───
// The API layer still calls these with the same names for backward
// compatibility. Under the hood, they query the 'organizations' table.

function createGroup({ name, inviteCode, teacherId }) {
  const stmt = db.prepare(`INSERT INTO organizations (name, type, invite_code, created_by) VALUES (@name, 'class', @inviteCode, @teacherId)`);
  return stmt.run({ name, inviteCode, teacherId }).lastInsertRowid;
}

function getGroupById(id) {
  return db.prepare("SELECT * FROM organizations WHERE id = ?").get(id);
}

function getGroupByCode(inviteCode) {
  return db.prepare("SELECT * FROM organizations WHERE invite_code = ?").get(inviteCode);
}

function joinGroup(orgId, userId) {
  const stmt = db.prepare(`INSERT OR IGNORE INTO org_members (org_id, user_id) VALUES (?, ?)`);
  stmt.run(orgId, userId);
}

function leaveGroup(orgId, userId) {
  db.prepare(`DELETE FROM org_members WHERE org_id = ? AND user_id = ?`).run(orgId, userId);
}

function getUserGroups(userId) {
  return db.prepare(`
    SELECT o.* FROM organizations o
    JOIN org_members om ON o.id = om.org_id
    WHERE om.user_id = ?
  `).all(userId);
}

function getGroupMembers(groupId) {
  return db.prepare(`
    SELECT u.id, u.display_name, u.role, om.joined_at 
    FROM users u
    JOIN org_members om ON u.id = om.user_id
    WHERE om.org_id = ?
    ORDER BY om.joined_at ASC
  `).all(groupId);
}

/**
 * Check if a display name is already used by a member of an organization.
 * Case-insensitive to prevent subtle impersonation (e.g., "Alex" vs "alex").
 */
function isDisplayNameTaken(orgId, displayName) {
  const row = db.prepare(`
    SELECT 1 FROM users u
    JOIN org_members om ON u.id = om.user_id
    WHERE om.org_id = ? AND LOWER(u.display_name) = LOWER(?)
    LIMIT 1
  `).get(orgId, displayName);
  return !!row;
}

/**
 * Close the database connection.
 */
function close() {
  if (db) db.close();
}

module.exports = {
  init,
  insertFractal,
  getFractals,
  getFractalById,
  getXpfData,
  incrementDownloads: (id) => db.prepare("UPDATE fractals SET downloads = downloads + 1 WHERE id = ?").run(id),
  incrementLikes,
  decrementLikes,
  createUser,
  getUserByEmail,
  createGroup,
  getGroupById,
  getGroupByCode,
  joinGroup,
  leaveGroup,
  getUserGroups,
  getGroupMembers,
  isDisplayNameTaken,
  close,
};
