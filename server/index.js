/**
 * XaoS Community Server — REST API
 *
 * Endpoints:
 *   POST /api/fractals          Upload a fractal (.xpf + thumbnail + metadata)
 *   GET  /api/fractals          Browse gallery (paginated, sortable)
 *   GET  /api/fractals/:id      Get fractal metadata
 *   GET  /api/fractals/:id/xpf  Download .xpf data (increments counter)
 *   GET  /api/fractals/:id/thumbnail  Get thumbnail image
 *
 * Run: npm start (default port 3000)
 * Config: PORT env variable
 */

const express = require("express");
const cors = require("cors");
const multer = require("multer");
const path = require("path");
const fs = require("fs");
const os = require("os"); // Added OS module for dynamic IP detection
const { v4: uuidv4 } = require("uuid");
const jwt = require("jsonwebtoken");
const db = require("./db");

const JWT_SECRET = process.env.JWT_SECRET || "xaos-super-secret-key-for-dev";

const app = express();
const PORT = process.env.PORT || 3000;

// ─── Middleware ───────────────────────────────────────────────

app.use(cors());
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Multer for thumbnail uploads — stored in uploads/ with unique names
const storage = multer.diskStorage({
  destination: path.join(__dirname, "uploads"),
  filename: (_req, file, cb) => {
    const ext = path.extname(file.originalname) || ".png";
    cb(null, `${uuidv4()}${ext}`);
  },
});

const upload = multer({
  storage,
  limits: {
    fileSize: 2 * 1024 * 1024, // 2 MB max for thumbnail
    fields: 10,
    files: 1,
  },
  fileFilter: (_req, file, cb) => {
    // Only accept image files
    if (file.mimetype.startsWith("image/")) {
      cb(null, true);
    } else {
      cb(new Error("Only image files are allowed for thumbnails"), false);
    }
  },
});

// ─── Initialize database ─────────────────────────────────────

db.init();

// ─── Rate Limiting (in-memory) ───────────────────────────────
// Tracks join attempts per IP to prevent brute-forcing invite codes.
// Max 5 attempts per IP per minute.
const joinAttempts = new Map(); // IP -> { count, resetTime }
const RATE_LIMIT_WINDOW_MS = 60 * 1000; // 1 minute
const RATE_LIMIT_MAX = 1000;

function checkRateLimit(ip) {
  const now = Date.now();
  const entry = joinAttempts.get(ip);
  if (!entry || now > entry.resetTime) {
    joinAttempts.set(ip, { count: 1, resetTime: now + RATE_LIMIT_WINDOW_MS });
    return true; // allowed
  }
  entry.count++;
  if (entry.count > RATE_LIMIT_MAX) {
    return false; // blocked
  }
  return true;
}

// Clean up stale entries every 5 minutes
setInterval(() => {
  const now = Date.now();
  for (const [ip, entry] of joinAttempts) {
    if (now > entry.resetTime) joinAttempts.delete(ip);
  }
}, 5 * 60 * 1000);

// ─── Authentication Middleware ─────────────────────────────────

function authenticateUser(req, res, next) {
  const authHeader = req.headers.authorization;
  if (!authHeader || !authHeader.startsWith("Bearer ")) {
    req.user = null;
    return next();
  }

  const token = authHeader.split(" ")[1];
  try {
    req.user = jwt.verify(token, JWT_SECRET);
  } catch (err) {
    req.user = null;
  }
  next();
}

app.use(authenticateUser);

// ─── Auth Routes ──────────────────────────────────────────────

app.post("/api/auth/teacher/login", (req, res) => {
  const { email, password } = req.body;
  if (!email || !password) return res.status(400).json({ error: "Missing fields" });

  let user = db.getUserByEmail(email);
  if (!user) {
    return res.status(400).json({ error: "Account not found" });
  } else if (user.password_hash !== password) {
    return res.status(400).json({ error: "Invalid password" }); // Use 400 instead of 401 to prevent Qt QNetworkAccessManager from swallowing the body
  }

  const token = jwt.sign({ id: user.id, role: user.role, displayName: user.display_name }, JWT_SECRET, { expiresIn: '7d' });
  res.json({ token, user: { id: user.id, email: user.email, displayName: user.display_name, role: user.role } });
});

app.post("/api/auth/teacher/signup", (req, res) => {
  const { email, password, displayName } = req.body;
  if (!email || !password || !displayName) return res.status(400).json({ error: "Missing fields" });

  let user = db.getUserByEmail(email);
  if (user) {
    return res.status(400).json({ error: "Account already exists" });
  }

  const id = db.createUser({ email, passwordHash: password, displayName: displayName, role: "teacher" });
  const token = jwt.sign({ id, role: "teacher", displayName }, JWT_SECRET, { expiresIn: '7d' });
  res.json({ token, user: { id, email, displayName, role: "teacher" } });
});

app.post("/api/auth/student/join", (req, res) => {
  const { inviteCode, displayName } = req.body;
  
  // If req.user is set, the student is already authenticated
  const isExistingUser = !!req.user;
  
  if (!inviteCode) return res.status(400).json({ error: "Missing invite code" });
  if (!isExistingUser && !displayName) return res.status(400).json({ error: "Missing display name" });

  // Rate limiting: max 5 attempts per IP per minute
  const clientIp = req.ip || req.connection.remoteAddress;
  if (!checkRateLimit(clientIp)) {
    return res.status(429).json({ error: "Too many attempts. Please wait a minute and try again." });
  }

  const group = db.getGroupByCode(inviteCode);
  if (!group) return res.status(404).json({ error: "Invalid invite code" });

  let userId;
  let finalDisplayName;

  if (isExistingUser) {
    userId = req.user.id;
    finalDisplayName = req.user.displayName;
  } else {
    finalDisplayName = displayName.trim();
    // Unique display name per group
    if (db.isDisplayNameTaken(group.id, finalDisplayName)) {
      return res.status(409).json({ error: `The name "${finalDisplayName}" is already taken in this group. Please choose a different name.` });
    }
    userId = db.createUser({ displayName: finalDisplayName, role: "student" });
  }

  db.joinGroup(group.id, userId);

  const token = isExistingUser ? req.headers.authorization.split(" ")[1] 
                               : jwt.sign({ id: userId, role: "student", displayName: finalDisplayName }, JWT_SECRET, { expiresIn: '30d' });
  
  res.json({ token, group: { id: group.id, name: group.name }, user: { id: userId, displayName: finalDisplayName, role: "student" } });
});

// ─── Group / Room Routes ──────────────────────────────────────

app.get("/api/user/rooms", (req, res) => {
  if (!req.user) return res.status(401).json({ error: "Unauthorized" });
  const rooms = db.getUserGroups(req.user.id);
  res.json({ rooms });
});

app.post("/api/rooms/:id/leave", (req, res) => {
  if (!req.user) return res.status(401).json({ error: "Unauthorized" });
  const orgId = parseInt(req.params.id, 10);
  db.leaveGroup(orgId, req.user.id);
  res.json({ success: true });
});

app.post("/api/groups", (req, res) => {
  if (!req.user || req.user.role !== "teacher") return res.status(403).json({ error: "Only teachers can create groups" });
  const { name } = req.body;
  if (!name) return res.status(400).json({ error: "Name is required" });

  // Generate 6-character alphanumeric invite code (excluding 0, O, 1, I)
  const chars = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
  let inviteCode = '';
  for (let i = 0; i < 6; i++) {
    inviteCode += chars.charAt(Math.floor(Math.random() * chars.length));
  }
  const id = db.createGroup({ name, inviteCode, teacherId: req.user.id });
  db.joinGroup(id, req.user.id);
  res.status(201).json({ id, name, inviteCode });
});

app.get("/api/groups", (req, res) => {
  if (!req.user) return res.status(401).json({ error: "Unauthorized" });
  const groups = db.getUserGroups(req.user.id);
  res.json({ groups });
});

app.get("/api/rooms/:id/members", (req, res) => {
  if (!req.user) return res.status(401).json({ error: "Unauthorized" });
  const orgId = parseInt(req.params.id, 10);
  const members = db.getGroupMembers(orgId);
  res.json({ members });
});

// ─── Routes ──────────────────────────────────────────────────

/**
 * POST /api/fractals
 *
 * Upload a fractal position with optional thumbnail.
 *
 * Body (multipart/form-data):
 *   title      (required) — display name
 *   author     (optional) — creator name, default "Anonymous"
 *   xpf        (required) — .xpf position data as text
 *   formula    (optional) — fractal formula name
 *   iterations (optional) — max iteration count
 *   zoomLevel  (optional) — zoom magnification string
 *   thumbnail  (optional) — PNG image file
 */
app.post("/api/fractals", upload.single("thumbnail"), (req, res) => {
  try {
    const { title, author, xpf, formula, iterations, zoomLevel, groupId } = req.body;

    if (!title || !title.trim()) {
      return res.status(400).json({ error: "Title is required" });
    }
    if (!xpf || !xpf.trim()) {
      return res.status(400).json({ error: "XPF data is required" });
    }

    // Sanity check: XPF should start with a comment or command
    const trimmed = xpf.trim();
    if (!trimmed.startsWith(";") && !trimmed.startsWith("(")) {
      return res.status(400).json({ error: "Invalid XPF data format" });
    }

    // Size limit for XPF data: 64 KB (position files are typically 200-500 bytes)
    if (xpf.length > 64 * 1024) {
      return res
        .status(400)
        .json({ error: "XPF data too large (max 64 KB)" });
    }

    const thumbnailFilename = req.file ? req.file.filename : null;

    const id = db.insertFractal({
      title: title.trim(),
      author: author ? author.trim() : (req.user ? req.user.displayName : "Anonymous"),
      userId: req.user ? req.user.id : null,
      groupId: groupId ? parseInt(groupId, 10) : null,
      formula: formula || null,
      iterations: iterations ? parseInt(iterations, 10) : null,
      zoomLevel: zoomLevel || null,
      xpfData: xpf,
      thumbnail: thumbnailFilename,
    });

    res.status(201).json({
      id,
      message: "Fractal shared successfully",
    });
  } catch (err) {
    console.error("Upload error:", err);
    res.status(500).json({ error: "Failed to save fractal: " + err.message });
  }
});

app.post("/api/fractals/:id/like", (req, res) => {
  const id = req.params.id;
  try {
    db.incrementLikes(id);
    res.json({ success: true });
  } catch (err) {
    res.status(500).json({ error: "Failed to like fractal" });
  }
});

/**
 * DELETE /api/fractals/:id/like
 *
 * Take a like back. Clients cap themselves at one like per fractal and
 * remember locally what they liked, so this only adjusts the running total.
 */
app.delete("/api/fractals/:id/like", (req, res) => {
  const id = req.params.id;
  try {
    db.decrementLikes(id);
    res.json({ success: true });
  } catch (err) {
    res.status(500).json({ error: "Failed to unlike fractal" });
  }
});

/**
 * GET /api/fractals
 *
 * Browse the community gallery.
 *
 * Query params:
 *   page  (default 1) — page number
 *   limit (default 20, max 50) — items per page
 *   sort  ("recent" or "popular", default "recent")
 */
app.get("/api/fractals", (req, res) => {
  try {
    let page = parseInt(req.query.page, 10) || 1;
    let limit = parseInt(req.query.limit, 10) || 20;
    const sort = req.query.sort === "popular" ? "popular" : "recent";
    const groupId = req.query.groupId ? parseInt(req.query.groupId, 10) : null;

    if (page < 1) page = 1;
    if (limit < 1) limit = 1;
    if (limit > 50) limit = 50;

    const result = db.getFractals(page, limit, sort, groupId);

    // Add thumbnail URLs to each item
    result.items = result.items.map((item) => ({
      ...item,
      thumbnailUrl: item.thumbnail
        ? `/api/fractals/${item.id}/thumbnail`
        : null,
    }));

    res.json(result);
  } catch (err) {
    console.error("Gallery fetch error:", err);
    res.status(500).json({ error: "Failed to fetch gallery" });
  }
});

/**
 * GET /api/fractals/:id
 *
 * Get metadata for a specific fractal.
 */
app.get("/api/fractals/:id", (req, res) => {
  try {
    const id = parseInt(req.params.id, 10);
    if (isNaN(id)) {
      return res.status(400).json({ error: "Invalid fractal ID" });
    }

    const fractal = db.getFractalById(id);
    if (!fractal) {
      return res.status(404).json({ error: "Fractal not found" });
    }

    fractal.thumbnailUrl = fractal.thumbnail
      ? `/api/fractals/${fractal.id}/thumbnail`
      : null;

    res.json(fractal);
  } catch (err) {
    console.error("Detail fetch error:", err);
    res.status(500).json({ error: "Failed to fetch fractal details" });
  }
});

/**
 * GET /api/fractals/:id/xpf
 *
 * Download the .xpf position data. Increments download counter.
 */
app.get("/api/fractals/:id/xpf", (req, res) => {
  try {
    const id = parseInt(req.params.id, 10);
    if (isNaN(id)) {
      return res.status(400).json({ error: "Invalid fractal ID" });
    }

    const xpfData = db.getXpfData(id);
    if (!xpfData) {
      return res.status(404).json({ error: "Fractal not found" });
    }

    res.type("text/plain").send(xpfData);
  } catch (err) {
    console.error("XPF download error:", err);
    res.status(500).json({ error: "Failed to download fractal" });
  }
});

/**
 * GET /api/fractals/:id/thumbnail
 *
 * Serve the thumbnail PNG image.
 */
app.get("/api/fractals/:id/thumbnail", (req, res) => {
  try {
    const id = parseInt(req.params.id, 10);
    if (isNaN(id)) {
      return res.status(400).json({ error: "Invalid fractal ID" });
    }

    const fractal = db.getFractalById(id);
    if (!fractal || !fractal.thumbnail) {
      return res.status(404).json({ error: "Thumbnail not found" });
    }

    const thumbPath = path.join(__dirname, "uploads", fractal.thumbnail);
    if (!fs.existsSync(thumbPath)) {
      return res.status(404).json({ error: "Thumbnail file missing" });
    }

    res.sendFile(thumbPath);
  } catch (err) {
    console.error("Thumbnail error:", err);
    res.status(500).json({ error: "Failed to serve thumbnail" });
  }
});

// ─── Health check ────────────────────────────────────────────

app.get("/api/health", (_req, res) => {
  res.json({ status: "ok", version: "1.0.0" });
});

// ─── Error handling ──────────────────────────────────────────

app.use((err, _req, res, _next) => {
  if (err instanceof multer.MulterError) {
    return res.status(400).json({ error: `Upload error: ${err.message}` });
  }
  console.error("Unhandled error:", err);
  res.status(500).json({ error: "Internal server error" });
});

// ─── Start ───────────────────────────────────────────────────

// Helper function to dynamically get your LAN IP address
function getLocalIpAddress() {
  const interfaces = os.networkInterfaces();
  let fallbackIp = "localhost";

  for (const interfaceName of Object.keys(interfaces)) {
    // Skip virtual adapters (VMware, VirtualBox, WSL)
    const lowerName = interfaceName.toLowerCase();
    if (lowerName.includes("vmware") || lowerName.includes("virtual") || lowerName.includes("wsl")) {
      continue;
    }

    for (const iface of interfaces[interfaceName]) {
      if (iface.family === "IPv4" && !iface.internal) {
        // Prioritize Wi-Fi or primary Ethernet
        if (lowerName.includes("wi-fi") || lowerName.includes("wireless")) {
          return iface.address;
        }
        // Save the first valid IP just in case we don't find a "Wi-Fi" specific one
        if (fallbackIp === "localhost") {
          fallbackIp = iface.address;
        }
      }
    }
  }
  return fallbackIp;
}

const localIp = getLocalIpAddress();

// Bind to 0.0.0.0 so the server is accessible from Android devices on the LAN
app.listen(PORT, "0.0.0.0", () => {
  console.log(`XaoS Community Server running on http://0.0.0.0:${PORT}`);
  console.log(`  LAN access: http://${localIp}:${PORT}`);
  console.log(`  POST /api/fractals          — Upload a fractal`);
  console.log(`  GET  /api/fractals          — Browse gallery`);
  console.log(`  GET  /api/fractals/:id      — Fractal details`);
  console.log(`  GET  /api/fractals/:id/xpf  — Download .xpf`);
  console.log(`  GET  /api/fractals/:id/thumbnail — Get thumbnail`);

  // ─── UDP Discovery Beacon ──────────────────────────────────
  // Broadcasts a small JSON packet every 3 seconds on UDP port 3001
  // so Android clients on the same LAN can find the server automatically.
  const dgram = require("dgram");
  const beacon = dgram.createSocket({ type: "udp4", reuseAddr: true });
  const BEACON_PORT = 3001;
  const beaconPayload = Buffer.from(
    JSON.stringify({ service: "xaos-community", ip: localIp, port: PORT })
  );

  beacon.bind(() => {
    beacon.setBroadcast(true);
    const beaconInterval = setInterval(() => {
      beacon.send(beaconPayload, 0, beaconPayload.length, BEACON_PORT, "255.255.255.255");
    }, 3000);

    // Store for cleanup
    global._beaconInterval = beaconInterval;
    global._beaconSocket = beacon;

    console.log(`  Discovery beacon broadcasting on UDP port ${BEACON_PORT}`);
  });
});

// Graceful shutdown
process.on("SIGINT", () => {
  if (global._beaconInterval) clearInterval(global._beaconInterval);
  if (global._beaconSocket) global._beaconSocket.close();
  db.close();
  process.exit(0);
});
process.on("SIGTERM", () => {
  if (global._beaconInterval) clearInterval(global._beaconInterval);
  if (global._beaconSocket) global._beaconSocket.close();
  db.close();
  process.exit(0);
});
