# XaoS Mobile (Android) - Community Feature

The mobile version of XaoS includes a built-in **Community Hub** that connects
to a REST API server for sharing and discovering fractals.

## Architecture

```
Mobile App (Qt/QML)          Community Server (Node.js)
+-----------------+          +------------------+
| CommunityClient |  HTTP    | Express REST API |
|   (C++ class)   | -------> | SQLite Database  |
| QML UI Screens  |          | File Storage     |
+-----------------+          +------------------+
```

### Key Files

| File | Description |
|------|-------------|
| `communityclient.h/cpp` | C++ HTTP client - handles all API calls |
| `server_config.h` | **Your private server URL** (gitignored) |
| `server_config.h.example` | Template - copy and configure |
| `qml/CommunityGallery.qml` | Main community hub screen |
| `qml/ShareDialog.qml` | Upload fractal dialog |
| `qml/TeacherDashboard.qml` | Teacher login/signup popup |
| `qml/JoinGroup.qml` | Student join room popup |
| `qml/CreateRoom.qml` | Teacher create room popup |
| `qml/StartScreen.qml` | Welcome screen with navigation |
| `qml/FractalDetail.qml` | Fractal preview and download |
| `qml/RoomMembersPopup.qml` | View members of a room |

## Setup for New Developers

### 1. Configure Server URL

Copy the example config and set your server address:

```bash
cd src/ui-mobile
cp server_config.h.example server_config.h
```

Edit `server_config.h`:
- For **local development** (Android emulator): use `http://10.0.2.2:3000`
- For **production** (cloud server): use `http://YOUR_SERVER_IP:3000`

### 2. Start the Server

```bash
cd server
npm install
npm start
```

### 3. Build the Mobile App

Open the project in Qt Creator, select your Android kit, and build.
The app will automatically connect to the URL defined in `server_config.h`.

## Community Feature Overview

### Public Gallery
- Anyone can browse and download shared fractals
- Sort by recent or popular
- Paginated grid view with thumbnails

### Private Rooms (Classrooms)
- **Teachers** can sign up, create rooms, and get a 6-character invite code
- **Students** join rooms using the invite code
- Invite codes use unambiguous characters (no 0/O or 1/I confusion)
- Fractals shared in a room are only visible to room members

### Sharing Flow
- When sharing a fractal, users can choose the target destination:
  - Public Gallery (visible to everyone)
  - Any room they belong to (visible to room members only)

## Server Discovery

The app uses two methods to find the server:

1. **UDP Beacon** (LAN only) - The server broadcasts on UDP port 3001.
   The app listens for this beacon to auto-discover local servers.
2. **Fallback URL** - If no beacon is received, the app falls back to the
   URL defined in `server_config.h`. This is the primary method for
   cloud-hosted servers.

## Error Handling

- Error messages auto-clear after 3.5 seconds
- Errors clear when switching tabs (e.g., Login to Signup)
- Server connection errors show user-friendly messages
- The app automatically reconnects when the server comes back online
