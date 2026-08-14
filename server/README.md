# XaoS Community Server

A lightweight REST API server for the XaoS fractal sharing community.
Users can upload fractal positions (`.xpf` files), browse community
creations, download fractals, and collaborate in private rooms.

## Features

- **Public Gallery** — Browse and share fractals with the world
- **Private Rooms** — Teachers can create invite-only classrooms
- **Authentication** — Teacher signup/login with JWT tokens
- **Student Access** — Join rooms via 6-character alphanumeric invite codes
- **Auto-Discovery** — UDP beacon on port 3001 for LAN discovery
- **Rate Limiting** — Built-in join-attempt throttling per IP

## Prerequisites

- **Node.js 20 LTS** (recommended) or Node.js 18+
- npm 10+

> **Warning**: `better-sqlite3@13` requires Node >= 22. If deploying on a
> low-memory server where Node 22 causes segfaults, install
> `better-sqlite3@11` instead (compatible with Node 20):
> ```bash
> npm install better-sqlite3@11
> ```

## Quick Start (Local Development)

```bash
cd server
npm install
npm start
```

The server starts on `http://localhost:3000` and broadcasts a UDP discovery
beacon on port 3001.

Set the `PORT` environment variable to use a different port:
```bash
PORT=8080 npm start
```

## API Endpoints

### Public

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/fractals` | Upload a fractal (multipart form) |
| `GET` | `/api/fractals` | Browse gallery (paginated) |
| `GET` | `/api/fractals/:id` | Get fractal metadata |
| `GET` | `/api/fractals/:id/xpf` | Download `.xpf` data |
| `GET` | `/api/fractals/:id/thumbnail` | Get thumbnail PNG |
| `GET` | `/api/health` | Health check |

### Authentication

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/auth/teacher/signup` | Register a teacher account |
| `POST` | `/api/auth/teacher/login` | Teacher login (returns JWT) |
| `POST` | `/api/auth/student/join` | Student joins a room via invite code |
| `POST` | `/api/auth/logout` | Logout (clears session) |

### Rooms

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/api/groups` | Create a room (teachers only) |
| `GET` | `/api/groups` | List rooms the user belongs to |
| `GET` | `/api/rooms/:id/members` | List members of a room |
| `POST` | `/api/rooms/:id/leave` | Leave a room |

### Upload (POST /api/fractals)

Multipart form data:
- `title` (required) - display name
- `author` (optional) - creator name, default 'Anonymous'
- `xpf` (required) - `.xpf` position data as text
- `formula` (optional) - fractal formula name
- `iterations` (optional) - max iteration count
- `zoomLevel` (optional) - zoom magnification string
- `thumbnail` (optional) - PNG image file (max 2 MB)
- `groupId` (optional) - post to a specific room instead of public gallery

### Browse (GET /api/fractals)

Query parameters:
- `page` (default: 1) - page number
- `limit` (default: 20, max: 50) - items per page
- `sort` - `recent` (default) or `popular` (by download count)

## Data Storage

- **Database**: SQLite file (`community.db`) in the server directory
- **Thumbnails**: PNG files in the `uploads/` directory

## Deployment to Oracle Cloud Free Tier

Oracle Cloud offers an **Always Free** ARM or x86 VM that is perfect for
hosting this server. Here is a complete guide based on real deployment
experience.

### 1. Create an Instance

- Shape: `VM.Standard.E2.1.Micro` (1 OCPU, 1 GB RAM)
- OS: Oracle Linux 9
- Download the SSH private key (`.key` file) during creation

### 2. SSH into the Instance

```bash
ssh -i /path/to/your-key.key opc@<PUBLIC_IP>
```

### 3. Add Swap Space (Important!)

The 1 GB RAM instance will freeze or kill processes when `dnf` tries to
refresh its package repos. **Create swap space first**:

```bash
sudo fallocate -l 3G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
echo '/swapfile swap swap defaults 0 0' | sudo tee -a /etc/fstab
```

### 4. Install Node.js

> **Do NOT use** `sudo dnf install nodejs` on 1 GB RAM instances - `dnf`
> will often freeze or OOM-kill itself while refreshing Oracle's large repos.
> Download the binary tarball instead:

```bash
curl -O https://nodejs.org/dist/v20.18.0/node-v20.18.0-linux-x64.tar.gz
tar -xzf node-v20.18.0-linux-x64.tar.gz
sudo cp -R node-v20.18.0-linux-x64/* /usr/local/
sudo ln -s /usr/local/bin/node /usr/bin/node
sudo ln -s /usr/local/bin/npm /usr/bin/npm
sudo ln -s /usr/local/bin/npx /usr/bin/npx
node --version
```

### 5. Install Build Tools (for better-sqlite3)

```bash
sudo dnf install -y gcc-c++ make python3
```

### 6. Open Firewall Ports

**Linux firewall:**
```bash
sudo firewall-cmd --zone=public --add-port=3000/tcp --permanent
sudo firewall-cmd --zone=public --add-port=3001/udp --permanent
sudo firewall-cmd --reload
```

**Oracle Cloud Console (separate layer!):**
1. Go to **Networking > Virtual Cloud Networks > your VCN**
2. Click your **subnet > Security Lists > Default Security List**
3. Click **Add Ingress Rules**
4. Source CIDR: `0.0.0.0/0`, Protocol: `TCP`, Port: `3000`
5. Save

### 7. Upload and Start the Server

From your local machine:
```bash
scp -i /path/to/key.key server_deploy.zip opc@<PUBLIC_IP>:~/
```

On the Oracle instance:
```bash
mkdir -p ~/server && cd ~/server
unzip -o ~/server_deploy.zip
npm install better-sqlite3@11
npm install
node index.js   # Test it works, then Ctrl+C
```

### 8. Keep It Running with PM2

```bash
sudo npm install -g pm2
pm2 start index.js --name "xaos-server"
pm2 save
pm2 startup
# Copy and run the sudo command it prints
```

### 9. Prevent Oracle from Reclaiming Idle Instances

Oracle may reclaim Always Free instances with consistently low CPU usage
(below ~10%). Create a keepalive script:

```bash
cat > ~/cpu_keepalive.sh << 'SCRIPT'
#!/bin/bash
while true; do
    timeout 30 dd if=/dev/urandom bs=1M count=100 | md5sum > /dev/null 2>&1
    sleep 300
done
SCRIPT
chmod +x ~/cpu_keepalive.sh
pm2 start ~/cpu_keepalive.sh --name "cpu-keepalive" --interpreter bash
pm2 save
```

### 10. Verify

```bash
curl http://localhost:3000/api/fractals
# From your browser: http://<PUBLIC_IP>:3000/api/fractals
```

## Known Issues and Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| `Segmentation fault` on startup | `better-sqlite3@13` + Node < 22 | `npm install better-sqlite3@11` |
| `dnf` freezes / OOM | Only 1 GB RAM | Add 3 GB swap first, or use binary tarball for Node |
| `sudo: npm: command not found` | Node installed to `/usr/local/bin` which `sudo` doesn't see | `sudo ln -s /usr/local/bin/npm /usr/bin/npm` |
| `Text file busy` error on copy | PM2 daemon holding old node binary | `pm2 kill` first, then copy |
| Browser can't connect | Oracle Cloud Security List missing | Add TCP port 3000 ingress rule in Cloud Console |
| Server stops after SSH disconnect | Not using a process manager | Use `pm2` to daemonize |

## License

GPL-2.0-or-later (same as XaoS)
