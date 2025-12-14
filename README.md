# SnowOwl

Language: English | [中文](README_zh-CN.md)

> SnowOwl 0.1.0 —— A multi-component video streaming pipeline with edge forwarding, RTMP/HLS distribution, and cross-platform monitoring capabilities.

## Table of Contents
- [SnowOwl](#snowowl)
  - [Table of Contents](#table-of-contents)
  - [Project Overview](#project-overview)
  - [Key Features](#key-features)
  - [Architecture](#architecture)
    - [Component Details](#component-details)
    - [Data Flow](#data-flow)
  - [Prerequisites](#prerequisites)
    - [Database Setup](#database-setup)
  - [Quick Start](#quick-start)
  - [Building Components](#building-components)
  - [Running Components](#running-components)
    - [Server](#server)
    - [Edge Device](#edge-device)
    - [Client](#client)
  - [Command Line Interface](#command-line-interface)
    - [Main Application (`owl`)](#main-application-owl)
    - [Management Tool (`owlctl`)](#management-tool-owlctl)
  - [APIs and Interfaces](#apis-and-interfaces)
  - [Project Structure](#project-structure)
  - [Frontend Development](#frontend-development)
  - [Application Scenarios](#application-scenarios)
  - [License](#license)

## Project Overview

SnowOwl is a comprehensive video streaming solution designed for real-time monitoring and analysis. It consists of multiple components working together to capture, process, distribute, and display video streams.

The system is composed of four main parts:

- **Edge Device** (`apps/edge_device`): Captures video from local cameras and forwards encoded frames to the server via TCP.
- **Central Server** (`apps/server`): Receives forwarded frames, performs analysis, stores events, and distributes streams via various protocols.
- **Client Applications** (`apps/client`): Provides multiple ways to consume the video streams including web and Flutter-based interfaces.
- **Command Line Tools** (`apps/cli`): Offers management and control capabilities for the entire system.

Current version: **v0.1.0**.

## Key Features

- 📡 **Multi-Protocol Streaming**: Supports RTMP, RTSP, HLS, WebRTC, and legacy TCP broadcast outputs
- 🖥️ **Cross-Platform Clients**: Native desktop clients (Windows, macOS, Linux) and web-based interfaces
- 📱 **Mobile Support**: Flutter-based mobile applications for Android and iOS
- 🔍 **Video Analysis**: Built-in motion detection, intrusion detection, and other computer vision capabilities
- 🗄️ **PostgreSQL Integration**: Robust database schema for device registry and metadata storage
- 🔧 **Unified CLI**: Single entry point for managing all components with rich command-line options
- 🌐 **RESTful API**: Comprehensive API for integration with external systems
- ⚡ **Real-time Communication**: WebSocket support for live updates and notifications

## Architecture

### Component Details

1. **Edge Device Agent (`snowowl-edge`)**:
   - Runs on embedded hardware (Raspberry Pi, etc.)
   - Captures video feeds from cameras or other sources
   - Encodes and streams video to the server via TCP
   - Registers itself in the device registry on startup

2. **Central Processing Server (`snowowl-server`)**:
   - Receives and decodes video streams from edge devices
   - Performs video analysis using computer vision techniques
   - Detects motion, intrusions, and other events
   - Stores events and metadata in PostgreSQL databases
   - Distributes streams via multiple protocols (RTMP, RTSP, HLS, WebRTC, TCP)
   - Provides REST and WebSocket APIs for clients

3. **Client Applications (`snowowl-client`)**:
   - Cross-platform desktop application (Windows, macOS, Linux)
   - Web-based interface for browser access
   - Mobile applications via Flutter
   - Displays live video feeds and historical events
   - Provides configuration interfaces for devices and alerts

4. **Command Line Tools**:
   - Main application (`owl`) for starting components
   - Management tool (`owlctl`) for system administration

### Data Flow

```
Camera → Edge Device → TCP Forwarding (7500)
        → Server → Stream Processing → Multi-Protocol Distribution
        → Client Applications (Web, Desktop, Mobile)
```

## Prerequisites

### Database Setup

SnowOwl uses PostgreSQL for device registry and metadata storage. Before running SnowOwl, you need to set up a local PostgreSQL database.

**Default configuration:**
- Database name: `snowowl_dev`
- Database user: `snowowl_dev`
- Database password: `SnowOwl_Dev!`

To set up the database:

1. Install PostgreSQL:
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install postgresql postgresql-contrib

   # macOS
   brew install postgresql

   # CentOS/RHEL
   sudo yum install postgresql-server postgresql-contrib
   ```

2. Start PostgreSQL service:
   ```bash
   # Ubuntu/Debian/CentOS
   sudo systemctl start postgresql
   sudo systemctl enable postgresql

   # macOS
   brew services start postgresql
   ```

3. Create the database and user:
   ```bash
   sudo -u postgres psql
   ```

   In the PostgreSQL shell, run:
   ```sql
   CREATE USER snowowl_dev WITH PASSWORD 'SnowOwl_Dev!';
   CREATE DATABASE snowowl_dev OWNER snowowl_dev;
   GRANT ALL PRIVILEGES ON DATABASE snowowl_dev TO snowowl_dev;
   \c snowowl_dev
   GRANT ALL PRIVILEGES ON SCHEMA public TO snowowl_dev;
   GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO snowowl_dev;
   GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO snowowl_dev;
   \q
   ```

The application will automatically create the required tables on first run.

## Quick Start

```bash
# 1. Start MediaMTX (keep terminal running) for RTMP/HLS support
mediamtx

# 2. Build binaries from repository root
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 3. Start server (new terminal)
./build/owl server start \
  --enable-rest --http-port 8081 \
  --enable-rtmp --rtmp-url rtmp://127.0.0.1:1935/live/stream \
  --config-db "postgresql://snowowl_dev:SnowOwl_Dev!@localhost/snowowl_dev"

# 4. Start edge device (another terminal)
./build/owl edge start

# 5. (Optional) Run web client
./build/owl client start --web --url="http://127.0.0.1:8081"
```

## Building Components

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

This will build all components including:
- Main application (`owl`)
- Server component (`snowowl-server`)
- Edge device component (`snowowl-edge`)
- Client component (`snowowl-client`)

## Running Components

### Server

Start the server with minimal configuration:

```bash
./build/owl server start --enable-rest --http-port 8081
```

Full-featured server with all protocols:

```bash
./build/owl server start \
  --enable-rest --http-port 8081 \
  --enable-rtmp --rtmp-url rtmp://127.0.0.1:1935/live/stream \
  --enable-rtsp --rtsp-url rtsp://127.0.0.1:8554/live/stream \
  --enable-hls --hls-playlist http://127.0.0.1:8888/hls/stream.m3u8 \
  --enable-webrtc \
  --config-db "postgresql://snowowl_dev:SnowOwl_Dev!@localhost/snowowl_dev"
```

### Edge Device

Start an edge device:

```bash
./build/owl edge start
```

With database connection:

```bash
./build/owl edge start --connect-database \
  --db-host localhost --db-port 5432 \
  --db-name snowowl_dev --db-user snowowl_dev \
  --db-password SnowOwl_Dev!
```

### Client

Start web client:

```bash
./build/owl client start --web --url="http://127.0.0.1:8081"
```

Start Flutter client:

```bash
./build/owl client start --flutter --device="MyDevice"
```

## Command Line Interface

### Main Application (`owl`)

The main application provides a unified entry point for all components:

```bash
# General help
./build/owl --help

# Component-specific help
./build/owl server start --help
./build/owl edge start --help
./build/owl client start --help
```

### Management Tool (`owlctl`)

The management tool provides administrative functions for a running system:

```bash
# List all registered devices
./build/owlctl --list-devices

# Get server status
./build/owlctl --server-status

# Manage configuration
./build/owlctl --list-config
./build/owlctl --get-config key
./build/owlctl --update-config key=value
./build/owlctl --reset-config

# Device management
./build/owlctl --register-device
./build/owlctl --device-info id
./build/owlctl --update-device id
./build/owlctl --delete-device id

# Stream control
./build/owlctl --start-stream device_id
./build/owlctl --stop-stream device_id
```

## APIs and Interfaces

- **REST API**: Available on `--http-port` (default 8081)
  - `GET /api/v1/capture/session/<id>` - Get session information
  - `GET /api/v1/devices` - List devices
  - `GET /api/v1/devices/<id>` - Get device information

- **WebSocket API**: Same port as REST API
  - Real-time device updates
  - Live streaming data
  - Event notifications

- **TCP Forwarding**: Port 7500 by default
  - Edge devices send encoded frames to the server
  - Format: 1-byte type + 4-byte little-endian length + data

- **Legacy TCP Broadcast**: Port 7000 by default
  - For backward compatibility
  - Broadcasts processed frames to connected clients

## Project Structure

```
.
├── apps/
│   ├── cli/                 # Command-line interface components
│   ├── client/              # Client applications
│   ├── edge_device/         # Edge device implementation
│   ├── server/              # Server implementation
│   └── main.cpp             # Main application entry point
├── frontend/
│   ├── flutter/             # Flutter client application
│   └── web/                 # Web client application
├── libs/                    # Shared libraries
├── plugins/                 # Extension plugins
├── config/                  # Configuration templates
├── docs/                    # Documentation
└── tests/                   # Unit and integration tests
```

## Frontend Development

### Web Client

The web client is a standard web application that connects to the server's REST and WebSocket APIs.

### Flutter Client

The Flutter client provides mobile and desktop applications. To set up the Flutter environment:

1. Install Flutter 3.22+
2. Navigate to the Flutter app directory:
   ```bash
   cd frontend/flutter/snowowl_app
   ```
3. Get dependencies:
   ```bash
   flutter pub get
   ```
4. Run the application:
   ```bash
   flutter run -d <device>
   ```

Recommended packages for Flutter development:
- `flutter_webrtc` for real-time video streaming
- `provider` or `riverpod` for state management
- `dio` for REST calls
- `web_socket_channel` for WebSocket communication

## Application Scenarios

SnowOwl is designed to be versatile and can be deployed in various environments:

### Industrial Monitoring
- Oil and gas pipeline inspection
- Equipment monitoring and maintenance
- Safety compliance monitoring
- Factory automation surveillance

### Home Security & Monitoring
- Home security systems
- Baby monitoring
- Pet surveillance
- Elderly care monitoring

### Commercial Applications
- Retail store monitoring
- Office security
- Warehouse inventory monitoring
- Parking lot surveillance

The modular architecture allows easy adaptation to different use cases while maintaining high performance and reliability.

## License

GNU General Public License v3.0, see [LICENSE](LICENSE) for details.