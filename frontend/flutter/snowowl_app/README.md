# SnowOwl Flutter App Skeleton

This folder will contain the Flutter client responsible for delivering a unified UI across Android, iOS, desktop, and web.

## Getting Started

1. Install Flutter 3.22+ with the desired platform targets enabled.
2. From the repository root run:
   ```bash
   cd frontend/flutter_app
   flutter create --platforms=android,ios,linux,macos,windows,web snowowl_flutter
   ```
3. Move the generated project contents back into this directory or keep the new subfolder according to your workspace preference.
4. Integrate with SnowOwl backend APIs (REST/WebSocket/WebRTC) defined under `docs/frontend_api_spec.md`.

## Suggested Packages

- `flutter_webrtc` for real-time video streaming.
- `provider` or `riverpod` for state management.
- `dio` for REST calls.
- `web_socket_channel` for server push events.

## Next Steps

- Define API interfaces in `lib/services/`.
- Implement shared UI components in `lib/widgets/`.
- Reuse theme constants to ensure consistent branding across desktop and mobile.
