# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DTodo Widget is a desktop todo widget application built with DTK6 (Deepin Tool Kit) and Qt6. It provides a macOS-style desktop widget with a frosted glass blur effect that stays on the desktop layer.

## Build Commands

```bash
# Build
mkdir -p build && cd build
cmake ..
make

# Run
./build/dtodo-widget
```

## Architecture

The application follows a three-layer architecture:

**DesktopWidget (main.cpp)** - Main window container that handles:
- Window management (frameless, stays-on-bottom, transparency)
- DPlatformWindowHandle integration for blur effects, rounded corners, shadows
- Drag-to-move functionality
- Edge-based resize with cursor updates
- Window geometry persistence via QSettings

**TodoWidget (todowidget.cpp/h)** - Central widget with DBlurEffectWidget base:
- Gaussian blur background effect
- Todo list UI (DLineEdit, DListView, DPushButton)
- CRUD operations for todos
- JSON persistence via QStandardPaths::AppDataLocation
- Item count tracking

**TodoItem (todoitem.cpp/h)** - Data model with id, text, completed status, and timestamp.

## Data Storage

- Todos: `~/.local/share/dtodo-widget/todos.json`
- Window geometry: `~/.config/dtodo-widget.conf`

## Dependencies

- Qt6 (Core, Widgets, Gui)
- DTK6 (Core, Gui, Widget)
- X11

## UI Behavior

- Double-click list item: toggle completion status (strikethrough effect)
- Drag window: move to any position
- Drag edges/corners: resize window (8px edge margin)
- Position and size auto-save on release
