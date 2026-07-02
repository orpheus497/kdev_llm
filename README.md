LLM KDevelop Plugin

AI assistant plugin designed specifically for the **KDevelop IDE** (KDE Frameworks 6). It integrates deeply into the `KDevelop::IPlugin` architecture, providing seamless, context-aware AI autocomplete, semantic refactoring, and conversational assistance natively within the IDE. Built for Tje Jenova Cognitive Architecture but can be configured for any LLM server on localhost:8080 or any other unique set API

## Features

- **Native KDevelop Integration**: Built from the ground up as a native `KDevelop::IPlugin`, seamlessly hooking into `KDevPlatform` components and UI controllers.
- **Deep Semantic Understanding (DUChain)**: Unlike basic text-editor plugins, JCA natively links with KDevelop's AST (Abstract Syntax Tree) engine. When you request a refactor, it parses your C++ selections using `DUChainUtils`, passing exact variable types and mathematical code context to the AI.
- **Project Awareness**: Directly accesses the `KDevelop::IProjectController` to automatically gather your active project name, root directory, and workspace data to provide the AI with macroscopic environmental awareness.
- **AI Autocomplete**: Fast, non-intrusive autocomplete suggestions injected directly into all open editor views via KDevelop's `IDocumentController` and `KTextEditor::Document` hooks.
- **In-Buffer Refactoring**: Edits and diffs are applied directly to the document, utilizing the `KDevelop::ContextMenuExtension` to provide an "AI: Refactor Selection" option in your standard editor right-click menu.
- **Local LLM Backend Support**: Communicates via standard HTTP POST APIs (like those provided by `llama.cpp`) for fast, local, and private inference.
- **Embedded Chat UI (Jenova C.A.)**: A lightweight, side-panel chat widget registered natively as a `KDevelop::IToolViewFactory` for interactive AI discussions alongside your code.

## Architecture

This plugin uses a purely native approach for maximum stability:
- **Zero Hacks**: Avoids XMLGUI overrides by utilizing native KDevelop Context Menus and ToolViews.
- **Global Deployment**: The `AiCompletionModel` dynamically attaches itself to all active and newly created text views across the entire IDE.
- **Memory Safety**: Uses memory-safe asynchronous `KTextEditor::MovingRange` tracking, ensuring thread-safe text edits that survive tab-switching or manual typing during an LLM request.

## Building and Installation

### Prerequisites

- KDE Frameworks 6 (KF6) headers (`ktexteditor-devel`, `kcoreaddons-devel`, `ki18n-devel`)
- KDevelop Platform headers (`kdevelop-devel` or `kdevplatform-devel`)
- Qt 6 headers (`qt6-base-devel`, `qt6-network-devel`)
- CMake >= 3.16
- Modern C++ Compiler (C++17/20)

### Compilation

We use the standard KDevPlatform plugin macros (`kdevplatform_add_plugin`) for correct KDE metadata discovery.

### Installation & Testing

To test the plugin locally in KDevelop (without requiring `sudo`):
```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local -DKDE_INSTALL_USE_QT_SYS_PATHS=OFF ..
make -j$(nproc)
make install
```
*(This installs the plugin object securely to `~/.local/lib/plugins/kdevplatform/65/jenovakdev.so` or similar depending on your KF6 version).*

**How to Test in KDevelop:**
1. Open KDevelop.
2. Go to **Settings -> Configure KDevelop... -> Plugins**.
3. Scroll through the list and check the box next to **JCA KDev Plugin** to enable it.
4. **Chat Sidebar**: Go to **Window -> Tool Views -> Jenova C.A.** to dock the chat panel to your IDE layout.
5. **AI Refactoring**: Select a block of text in the editor, right-click it, and select **AI: Refactor Selection...**.
6. **Autocomplete**: Begin typing in any C++ or text file to see the AI generate code suggestions in the native autocomplete dropdown.

## License

This project is licensed under the BSD 2-Clause License - see the [LICENSE](LICENSE) file for details.
