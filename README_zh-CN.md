# SnowOwl

语言：中文 | [English](README.md)

> SnowOwl 0.1.0 —— 多组件视频流管道，集成了边缘转发、RTMP/HLS 分发和跨平台监控功能。

## 目录
- [SnowOwl](#snowowl)
  - [目录](#目录)
  - [项目概述](#项目概述)
  - [主要特性](#主要特性)
  - [系统架构](#系统架构)
    - [组件详情](#组件详情)
    - [数据流](#数据流)
  - [环境要求](#环境要求)
    - [数据库设置](#数据库设置)
  - [快速开始](#快速开始)
  - [构建组件](#构建组件)
  - [运行组件](#运行组件)
    - [服务器](#服务器)
    - [边缘设备](#边缘设备)
    - [客户端](#客户端)
  - [命令行界面](#命令行界面)
    - [主应用 (`owl`)](#主应用-owl)
    - [管理工具 (`owlctl`)](#管理工具-owlctl)
  - [API 和接口](#api-和接口)
  - [项目结构](#项目结构)
  - [前端开发](#前端开发)
  - [应用场景](#应用场景)
  - [许可证](#许可证)

## 项目概述

SnowOwl 是一个综合性的视频流解决方案，专为实时监控和分析而设计。它由多个协同工作的组件组成，用于捕获、处理、分发和显示视频流。

系统由四个主要部分组成：

- **边缘设备** (`apps/edge_device`)：从本地摄像头捕获视频并通过 TCP 将编码帧转发到服务器。
- **中央服务器** (`apps/server`)：接收转发的帧，执行分析，存储事件，并通过各种协议分发流。
- **客户端应用** (`apps/client`)：提供多种方式来消费视频流，包括网页和基于 Flutter 的界面。
- **命令行工具** (`apps/cli`)：为整个系统提供管理和控制功能。

当前版本：**v0.1.0**。

## 主要特性

- 📡 **多协议流媒体**：支持 RTMP、RTSP、HLS、WebRTC 和传统 TCP 广播输出
- 🖥️ **跨平台客户端**：原生桌面客户端（Windows、macOS、Linux）和基于网页的界面
- 📱 **移动支持**：基于 Flutter 的移动应用，支持 Android 和 iOS
- 🔍 **视频分析**：内置运动检测、入侵检测和其他计算机视觉功能
- 🗄️ **PostgreSQL 集成**：强大的数据库模式用于设备注册和元数据存储
- 🔧 **统一 CLI**：单个入口点管理所有组件，具有丰富的命令行选项
- 🌐 **RESTful API**：全面的 API 用于与外部系统集成
- ⚡ **实时通信**：WebSocket 支持实时更新和通知

## 系统架构

### 组件详情

1. **边缘设备代理 (`snowowl-edge`)**：
   - 在嵌入式硬件上运行（如 Raspberry Pi 等）
   - 从摄像头或其他来源捕获视频流
   - 编码并通过 TCP 将视频流传输到服务器
   - 启动时在设备注册表中注册自己

2. **中央处理服务器 (`snowowl-server`)**：
   - 接收和解码来自边缘设备的视频流
   - 使用计算机视觉技术执行视频分析
   - 检测运动、入侵和其他事件
   - 在 PostgreSQL 数据库中存储事件和元数据
   - 通过多种协议分发流（RTMP、RTSP、HLS、WebRTC、TCP）
   - 为客户端提供 REST 和 WebSocket API

3. **客户端应用 (`snowowl-client`)**：
   - 跨平台桌面应用（Windows、macOS、Linux）
   - 基于网页的界面用于浏览器访问
   - 移动应用通过 Flutter 实现
   - 显示实时视频流和历史事件
   - 提供设备和警报的配置界面

4. **命令行工具**：
   - 主应用 (`owl`) 用于启动组件
   - 管理工具 (`owlctl`) 用于系统管理

### 数据流

```
摄像头 → 边缘设备 → TCP 转发 (7500)
        → 服务器 → 流处理 → 多协议分发
        → 客户端应用 (网页、桌面、移动端)
```

## 环境要求

### 数据库设置

SnowOwl 使用 PostgreSQL 进行设备注册和元数据存储。在运行 SnowOwl 之前，您需要设置一个本地 PostgreSQL 数据库。

**默认配置：**
- 数据库名：`snowowl_dev`
- 数据库用户：`snowowl_dev`
- 数据库密码：`SnowOwl_Dev!`

设置数据库的步骤：

1. 安装 PostgreSQL：
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install postgresql postgresql-contrib

   # macOS
   brew install postgresql

   # CentOS/RHEL
   sudo yum install postgresql-server postgresql-contrib
   ```

2. 启动 PostgreSQL 服务：
   ```bash
   # Ubuntu/Debian/CentOS
   sudo systemctl start postgresql
   sudo systemctl enable postgresql

   # macOS
   brew services start postgresql
   ```

3. 创建数据库和用户：
   ```bash
   sudo -u postgres psql
   ```

   在 PostgreSQL 命令行中，运行：
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

应用程序会在首次运行时自动创建所需的表。

## 快速开始

```bash
# 1. 启动 MediaMTX（保持终端运行）以支持 RTMP/HLS
mediamtx

# 2. 在仓库根目录构建二进制文件
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 3. 启动服务器（新建终端）
./build/owl server start \
  --enable-rest --http-port 8081 \
  --enable-rtmp --rtmp-url rtmp://127.0.0.1:1935/live/stream \
  --config-db "postgresql://snowowl_dev:SnowOwl_Dev!@localhost/snowowl_dev"

# 4. 启动边缘设备（另一个终端）
./build/owl edge start

# 5. （可选）运行网页客户端
./build/owl client start --web --url="http://127.0.0.1:8081"
```

## 构建组件

从仓库根目录：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

这将构建所有组件，包括：
- 主应用 (`owl`)
- 服务器组件 (`snowowl-server`)
- 边缘设备组件 (`snowowl-edge`)
- 客户端组件 (`snowowl-client`)

## 运行组件

### 服务器

使用最小配置启动服务器：

```bash
./build/owl server start --enable-rest --http-port 8081
```

全功能服务器（支持所有协议）：

```bash
./build/owl server start \
  --enable-rest --http-port 8081 \
  --enable-rtmp --rtmp-url rtmp://127.0.0.1:1935/live/stream \
  --enable-rtsp --rtsp-url rtsp://127.0.0.1:8554/live/stream \
  --enable-hls --hls-playlist http://127.0.0.1:8888/hls/stream.m3u8 \
  --enable-webrtc \
  --config-db "postgresql://snowowl_dev:SnowOwl_Dev!@localhost/snowowl_dev"
```

### 边缘设备

启动边缘设备：

```bash
./build/owl edge start
```

带数据库连接：

```bash
./build/owl edge start --connect-database \
  --db-host localhost --db-port 5432 \
  --db-name snowowl_dev --db-user snowowl_dev \
  --db-password SnowOwl_Dev!
```

### 客户端

启动网页客户端：

```bash
./build/owl client start --web --url="http://127.0.0.1:8081"
```

启动 Flutter 客户端：

```bash
./build/owl client start --flutter --device="MyDevice"
```

## 命令行界面

### 主应用 (`owl`)

主应用为所有组件提供了统一的入口：

```bash
# 通用帮助
./build/owl --help

# 组件特定的帮助
./build/owl server start --help
./build/owl edge start --help
./build/owl client start --help
```

### 管理工具 (`owlctl`)

管理工具为运行中的系统提供管理功能：

```bash
# 列出所有已注册的设备
./build/owlctl --list-devices

# 获取服务器状态
./build/owlctl --server-status

# 管理配置
./build/owlctl --list-config
./build/owlctl --get-config key
./build/owlctl --update-config key=value
./build/owlctl --reset-config

# 设备管理
./build/owlctl --register-device
./build/owlctl --device-info id
./build/owlctl --update-device id
./build/owlctl --delete-device id

# 流控制
./build/owlctl --start-stream device_id
./build/owlctl --stop-stream device_id
```

## API 和接口

- **REST API**：在 `--http-port` 上可用（默认 8081）
  - `GET /api/v1/capture/session/<id>` - 获取会话信息
  - `GET /api/v1/devices` - 列出设备
  - `GET /api/v1/devices/<id>` - 获取设备信息

- **WebSocket API**：与 REST API 使用相同端口
  - 实时设备更新
  - 实时流数据
  - 事件通知

- **TCP 转发**：默认端口 7500
  - 边缘设备将编码帧发送到服务器
  - 格式：1字节类型 + 4字节小端长度 + 数据

- **传统 TCP 广播**：默认端口 7000
  - 用于向后兼容
  - 向连接的客户端广播处理后的帧

## 项目结构

```
.
├── apps/
│   ├── cli/                 # 命令行界面组件
│   ├── client/              # 客户端应用
│   ├── edge_device/         # 边缘设备实现
│   ├── server/              # 服务器实现
│   └── main.cpp             # 主应用入口点
├── frontend/
│   ├── flutter/             # Flutter 客户端应用
│   └── web/                 # 网页客户端应用
├── libs/                    # 共享库
├── plugins/                 # 扩展插件
├── config/                  # 配置模板
├── docs/                    # 文档
└── tests/                   # 单元测试和集成测试
```

## 前端开发

### 网页客户端

网页客户端是一个标准的网页应用，连接到服务器的 REST 和 WebSocket API。

### Flutter 客户端

Flutter 客户端提供移动和桌面应用。设置 Flutter 环境：

1. 安装 Flutter 3.22+
2. 导航到 Flutter 应用目录：
   ```bash
   cd frontend/flutter/snowowl_app
   ```
3. 获取依赖：
   ```bash
   flutter pub get
   ```
4. 运行应用：
   ```bash
   flutter run -d <device>
   ```

推荐的 Flutter 开发包：
- `flutter_webrtc` 用于实时视频流
- `provider` 或 `riverpod` 用于状态管理
- `dio` 用于 REST 调用
- `web_socket_channel` 用于 WebSocket 通信

## 应用场景

SnowOwl 设计灵活，可部署在各种环境中：

### 工业监控
- 石油天然气管道巡检
- 设备监控与维护
- 安全合规监控
- 工厂自动化监控

### 家庭安全与监控
- 家庭安防系统
- 婴儿监护
- 宠物监控
- 老人看护监控

### 商业应用
- 零售店监控
- 办公室安全
- 仓库库存监控
- 停车场监控

模块化架构允许轻松适应不同的使用场景，同时保持高性能和可靠性。

## 许可证

GNU General Public License v3.0，详见 [LICENSE](LICENSE)。