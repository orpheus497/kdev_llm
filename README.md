# Jenova K Text

Jenova K Text is a native AI assistant plugin for KDE Frameworks 6 (KF6) text editors, such as Kate and KDevelop. It integrates deeply into the `KTextEditor` architecture, providing seamless, context-aware AI autocomplete, refactoring, and conversational assistance without invasive custom UI popups.

## Features

- **Native KTextEditor Integration**: Operates natively as a `ktexteditor` plugin, detected automatically by KDE applications via KDE Frameworks 6 plugin discovery.
- **AI Autocomplete**: Fast, non-intrusive autocomplete suggestions injected directly into Kate's standard code completion UI (`KTextEditor::CodeCompletionModel`).
- **In-Buffer Refactoring**: Edits and diffs are applied directly to the `KTextEditor::Document`, leveraging the IDE's built-in Git Diff tools and `Ctrl+Z` undo functionality.
- **Local LLM Backend Support**: Communicates via standard HTTP POST APIs (like those provided by `llama.cpp`) for fast, local, and private inference.
- **Embedded Chat UI**: A lightweight, side-panel chat widget (ToolView) for interactive AI discussions, featuring Markdown support and automatic code formatting.

## Architecture

This plugin uses a purely native approach:
- **Zero Hacks**: Relies strictly on `KTextEditor` APIs for modifications and avoids custom floating windows.
- **Memory Safety**: Uses memory-safe asynchronous `KTextEditor::MovingRange` tracking, ensuring thread-safe text edits that survive the user switching tabs or altering the text during an LLM request.

## Building and Installation

### Prerequisites

- KDE Frameworks 6 (KF6) headers (`ktexteditor-devel`, `kcoreaddons-devel`, `ki18n-devel`, `kxmlgui-devel`)
- Qt 6 headers (`qt6-base-devel`, `qt6-network-devel`)
- CMake >= 3.16
- Modern C++ Compiler (C++17/20)

### Compilation

We use the standard KCoreAddons macro (`kcoreaddons_add_plugin`) to ensure correct metadata generation and plugin caching:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Installation

For local testing in Kate:
```bash
make install
```
This typically installs the plugin object to `~/.local/lib/plugins/kf6/ktexteditor/jenovaktext.so`. You may need to restart Kate for the plugin cache to discover it.

## License

This project is licensed under the BSD 2-Clause License - see the [LICENSE](LICENSE) file for details.
