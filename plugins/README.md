# Plugins Directory

This directory contains plugins that extend the functionality of SnowOwl.

## Plugin Structure

Each plugin should follow this structure:
```
plugin_name/
├── manifest.json          # Plugin metadata
├── lib/                   # Compiled plugin libraries
├── src/                   # Plugin source code
├── assets/                # Plugin assets (images, configs, etc.)
└── README.md              # Plugin-specific documentation
```

## Plugin Types

1. **Server Plugins** - Extend server-side functionality
2. **Edge Plugins** - Extend edge device functionality
3. **Client Plugins** - Extend client UI functionality

## Creating a New Plugin

To create a new plugin, follow these steps:
1. Create a new directory with your plugin name
2. Create a manifest.json file with plugin metadata
3. Implement your plugin according to the plugin interface
4. Place compiled libraries in the lib/ directory

## Loading Plugins

Plugins are automatically loaded at startup based on their manifest files.