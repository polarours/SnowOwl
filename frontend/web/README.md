# SnowOwl Web Frontend Skeleton

This directory is reserved for the SPA that mirrors the Flutter app capabilities for pure browser usage.

Recommended stack:

- Vite + React + TypeScript.
- TailwindCSS or MUI for layout consistency.
- `@microsoft/signalr` or native WebSocket for the alert feed.
- Reuse WebRTC/HLS modules shared with the Flutter app when possible.

## Bootstrapping

```bash
cd frontend/web
npm create vite@latest snowowl-web -- --template react-ts
```

After scaffolding, align API models with `docs/frontend/api_plan.md`.
