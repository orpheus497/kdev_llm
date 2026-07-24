**Timestamp**: 2026-07-24 10:07

## Qt6 UI: Explicit QFlags Type Casting for Markdown Features
* **Decision**: Wrapped `QTextDocument::MarkdownDialectGitHub | QTextDocument::MarkdownNoHTML` in `QTextDocument::MarkdownFeatures(...)` inside `ChatMessageDelegate::createDoc()`.
* **Justification**: In Qt 6, bitwise OR on enum values evaluates to `int`, which triggers strict type checking errors when passed to functions taking `QFlags<T>`. Explicitly constructing `QTextDocument::MarkdownFeatures` satisfies GCC type rules without `-fpermissive`.

**Timestamp**: 2026-07-24 10:04

## Architecture: Structural Normalization & Self-Contained Qt Headers
* **Decision**: Sanitized `src/context/ContextManager.h` and `src/ui/ChatMessageDelegate.h` by removing all stashed Git conflict markers and ensuring explicit `#include <QCache>`, `#include <QHash>`, `#include <QFileSystemWatcher>`, and `#include <QTextDocument>` header directives.
* **Justification**: Completely eliminates preprocessor and Qt MOC syntax failures caused by Git conflict markers. Guarantees complete class definitions for template types (`QCache<QString, QTextDocument>`), restoring 100% build compatibility for MOC and GCC compilation without temporary hotfixes.

**Timestamp**: 2026-07-24 10:00

## Build & Header Inclusions: Explicit Include Directives for Qt Header Self-Containment
* **Decision**: Added `#include <QMessageBox>` to `src/ui/AiChatWidget.h`, `#include <QTextDocument>` to `src/ui/ChatMessageDelegate.h`, and stripped unresolved version control conflict markers from `src/context/ContextManager.h`.
* **Justification**: Cleaned merge markers to allow C++ preprocessor/MOC parsing. Solved template instantiation errors (`QCache<QString, QTextDocument>`) by providing full type definition of `QTextDocument` in the header. Provided `QMessageBox` definition for virtual deletion prompt signature.

**Timestamp**: 2026-07-22 13:45

## UI: Explicit Default Button for Deletion Confirmation
* **Decision**: Set the default button in `QMessageBox::question` explicitly to `QMessageBox::No`. Added standard `// ##Action purpose` and verified `// ##Step purpose` and `// ##Condition purpose` annotations.
* **Justification**: Setting the default button to "No" prevents accidental destructive actions when the dialog is prompt-focused and immediately confirmed (e.g. by hitting enter). Annotations conform to codebase-wide prefix guidelines.

**Timestamp**: 2026-07-22 13:41

## Testing: Headless-Safe Deletion Prompts
* **Decision**: Refactored the interactive `QMessageBox::question` dialog in `AiChatWidget::deleteCurrentConversation()` into a virtual protected method `askDeletionConfirmation()`. Overrode this method in a subclass `TestableAiChatWidget` inside the test suite to bypass GUI blocking.
* **Justification**: Executing blocking GUI prompts during automated tests in offscreen environments (`QT_QPA_PLATFORM=offscreen`) stalls the pipeline indefinitely. Abstracting the confirmation into a virtual method allows clean mocking and full coverage of the deletion codepath without requiring user input.

**Timestamp**: 2026-07-06 06:49\n* Successfully untracked massive local build cache directory (over 45k files) that was previously polluting the repository state.\n* Restructured the src/CMakeLists.txt to properly embed JSON metadata using KDevelop's kdevplatform_add_plugin macro, restoring standard KDevelop KF6 plugin discovery conventions.\n* Removed non-standard hotfix instructions from the README, restoring a standard KDE CMake build command set.\n\n**Timestamp**: 2026-07-03 04:14

## UI: Dynamic Layouts & Native Wrapping
* **Decision**: Removed `setMinimumHeight` and `setMaximumHeight` from `AiChatInputWidget`'s text box, utilizing `QSizePolicy::Expanding` and `QSizePolicy::MinimumExpanding` instead. Updated `AiChatWidget`'s vertical layout to assign a stretch factor of `1` to the chat history and `0` to the input widget.
* **Justification**: Hardcoded pixel sizing breaks IDE layouts on different resolutions and DPI settings, resulting in a "blocky" UI. By relying entirely on KDevelop's layout engine and Qt stretch factors, the sidebar natively expands and contracts seamlessly.

**Timestamp**: 2026-07-03 04:12

## Architecture: Project Rebranding to KDev LLM
* **Decision**: Renamed core plugin files `JenovaPlugin` and `jenovakdev.json` to `KDevLLMPlugin` and `kdevllm.json`, updated all string literals, JSON plugin descriptors, and root `CMakeLists.txt` targets.
* **Justification**: To fulfill user request to align the plugin's branding explicitly with "KDev LLM". Using shell renaming before updating CMake targets prevented dangling references.

**Timestamp**: 2026-07-03 02:01

## Architecture: Native IDE Project & DUChain Integration
* **Decision**: Upgraded `ContextManager.cpp` to pull data from `KDevelop::ICore::self()->projectController()` and `KDevelop::DUChain`. Used `KDevelop::DUChainUtils::itemUnderCursor` to extract semantic context (e.g. types and declarations) instead of parsing raw text, and pulled full project scope data. Fixed `ToolView` initialization by explicitly calling `CreateAndRaise`.
* **Justification**: The user demanded a *true* IDE integration, not just a Kate editor port. By linking against `KDev::Project` and `KDev::Language` and utilizing the AST Engine, the LLM now understands C++ types and project scopes semantically.

# Decisions Log

**Timestamp**: 2026-07-03 01:42

## Architecture: KDevelop Context Menus & Autocomplete
* **Decision**: Implement `KDevelop::ContextMenuExtension` inside `JenovaPlugin` instead of overriding `KTextEditor::MainWindow` XMLGUI files, and register `AiCompletionModel` globally by hooking into `KTextEditor::Editor::instance()->documentCreated`.
* **Justification**: This maps the Kate XMLGUI approach and model registration to native, clean KDevPlatform mechanisms. `ContextMenuExtension` natively hooks into KDevelop's right-click menus, while the `KTextEditor::Editor` global signals safely deploy our autocomplete model to all editors.

**Timestamp**: 2026-07-03 01:38

## Architecture: KDevelop ToolView
* **Decision**: Implemented `KDevelop::IToolViewFactory` to register `AiChatWidget` via `core()->uiController()->addToolView()`.
* **Justification**: This maps the Kate `ToolView` concept to the native KDevPlatform architecture while preserving the inner chat UI logic.

**Timestamp**: 2026-07-03 01:34

## Architecture: KDevelop Ground-Up Rebuild
* **Decision**: Pivot from `KTextEditor::Plugin` to `KDevelop::IPlugin` and scaffold in a parallel `src_kdevelop` directory.
* **Justification**: The user explicitly instructed that the project must be a proper KDevelop plugin rather than a Kate plugin, and demanded a complete ground-up rebuild without deleting the old files until the new plugin is fully functional.

## Architecture: Native KTextEditor Menus
* **Decision**: Replace `QMenu::addAction` in the `contextMenuAboutToShow` signal with native XMLGUI `<Menu name="ktexteditor_popup">`.
* **Justification**: Using KDE's XMLGUI framework correctly integrates the action into Kate/KDevelop without fragile code hooks, adhering strictly to the official KTextEditor plugin development process and preventing "reinventing the wheel".

## UX: Refactor Prompting
* **Decision**: Ask the user for explicit instructions via `QInputDialog` during a refactor request rather than silently issuing a generic "Refactor this code" prompt.
* **Justification**: AI refactoring without constraints leads to hallucinations and invalid output (e.g., C segfault examples). Letting the user specify the operation (e.g., "Extract function", "Optimize") and injecting full file context resolves logic gaps and transforms the placeholder feature into a functional tool.

**Timestamp**: 2026-07-02 15:21

## Architecture: Reference Replication
* **Decision**: Extract `CommandTextEdit` into `AiChatInputWidget` and implement context menu hooks instead of deleting original classes or force-swapping whole codebases.
* **Justification**: Dumping an external repo into our project breaks FOSS rules and introduces massive tech debt. Integrating their UI logic (e.g. `@file` mentions and right-click menus) into our native classes preserves our clean `QNetworkAccessManager` backend and guarantees separation of concerns.

## UI: Code Completion Crash Fixes
* **Decision**: Remove manual `rowCount()` overrides. Rely strictly on KTextEditor's `setRowCount()` API natively synchronized with `beginResetModel()`.
* **Justification**: KTextEditor's base `CodeCompletionModel` utilizes complex internal Qt tree-structures based around its own internal `d->rowCount` pointer. Overriding the `rowCount()` function without using `setRowCount()` causes a severe data de-synchronization where KTextEditor attempts to iterate over internal arrays that have size 0 while the override falsely reports size 1, leading to immediate recursion loops and segmentation faults during UI rendering. Wrapping `m_completions` changes inside `beginResetModel()` completely stabilizes it.

## Network: QNetworkReply Abortion Lifecycle
* **Decision**: Remove `deleteLater()` calls immediately following `abort()` in `LlamaClient` network requests.
* **Justification**: When `QNetworkReply::abort()` is called, Qt synchronously emits the `finished()` signal on the main event loop thread before `abort()` returns. Our `finished()` slot natively captures this and sets the pointer to `nullptr` while calling `deleteLater()`. Executing a second `deleteLater()` immediately after `abort()` in the calling function results in a null pointer dereference (`nullptr->deleteLater()`), instantly crashing the application.

**Timestamp**: 2026-07-02 14:43

## Architecture: LSP and Interpretation
* **Decision**: Retain Kate's basic `CodeCompletionModel` instead of writing a full out-of-process LSP server. Retain basic context aggregation instead of full project AST parsing.
* **Justification**: A Qt Plugin is fundamentally restricted from acting as an LSP server. Deep parsing is beyond the scope of a fast inference client. We mitigate this by ensuring the plugin operates correctly within its intended scope.


**Timestamp**: 2026-07-02 13:33

## Architecture: KF6 & Qt6 Native Plugin
* **Decision**: Target KDE Frameworks 6 exclusively.
* **Justification**: Modern KDE applications (Kate, KDevelop) have migrated to Qt6/KF6.
* **Impact**: Requires `kf6-*` packages and modern C++17/20.

## Backend: Local Llama.cpp REST API
* **Decision**: Communicate via HTTP POST using `QNetworkAccessManager`.
* **Justification**: Llama.cpp provides standard `/completion` and `/chat/completions` endpoints.

## Network: Async QNetworkReply Handling
* **Decision**: Overwrite ongoing requests by calling `abort()` and `deleteLater()` on the old pointers, and use `sender()` in slots.
* **Justification**: Prevents crashes or signal cross-talk when multiple requests (e.g., chat vs refactor) overlap asynchronously in `LlamaClient`.

## User Interface: Chat Interactions
* **Decision**: Implement `QEventFilter` on `QTextEdit` to capture `Enter` for sending messages, and auto-populate a welcome guide on load.
* **Justification**: Empty chat panels feel like non-functional stubs. Intercepting `Enter` matches standard chat UX and greatly enhances usability.

## Architecture: Native Integration
* **Decision**: Remove custom `ComparisonView` popup. Pipe LLM edits directly into `KTextEditor::Document` via `replaceText`.
* **Justification**: Custom popups fight the host IDE. Editing the native buffer lets the user rely on the IDE's built-in Git Diff and Undo functionality, shifting from a "slap-on app" to a deeply integrated native plugin.
