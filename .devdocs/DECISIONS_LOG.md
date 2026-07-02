# Decisions Log

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
