# Sentinel Security Journal
## 2024-07-07 - QTextBrowser URL Security
**Vulnerability:** QTextBrowser was configured with `setOpenExternalLinks(true)`, which automatically passes clicked markdown links to the OS.
**Learning:** LLMs can generate malicious markdown links with arbitrary URI schemes (e.g., `file://`, `javascript:`, or OS-specific app links). If clicked by the user, this can lead to arbitrary file reads or code execution.
**Prevention:** Disable `openExternalLinks` and `openLinks`. Manually connect the `anchorClicked` signal to a custom slot that validates the URL scheme (allowing only `http` and `https`) before calling `QDesktopServices::openUrl()`. Additionally, manually handle relative URLs with fragments by calling `scrollToAnchor()` to preserve internal anchor navigation.
## 2023-10-27 - [Security Vulnerability] Unsanitized Markdown Rendering of LLM Output
**Vulnerability:** XSS/HTML Injection in QTextBrowser via Markdown
**Learning:** `QTextBrowser::setMarkdown` enables HTML rendering by default. Thus, when passing untrusted output to it, any embedded HTML tags are rendered, which can lead to XSS/Script Execution in Qt.
**Prevention:** Configure the underlying `QTextDocument` directly by passing `QTextDocument::MarkdownNoHTML` to `QTextDocument::setMarkdown()`. This disables rendering raw HTML while keeping Markdown parsing active.

## 2024-05-24 - Infinite Loop DoS in Directory Traversal
**Vulnerability:** An infinite loop DoS vector was found in `ContextManager::getProjectRoot` during directory traversal.
**Learning:** Comparing a path string to `QStringLiteral("/")` to stop traversal fails on non-Unix OSs (like Windows where root is `C:/`) or if `dir.cdUp()` fails silently, causing the loop to run indefinitely while consuming CPU resources.
**Prevention:** When traversing up a directory tree using `QDir`, always rely on `QDir::isRoot()` as the loop terminator and check the return value of `dir.cdUp()` to safely break on traversal failure.
<<<<<<< HEAD
=======

## 2024-07-21 - XSS/HTML Injection in QTextDocument Markdown Parsing
**Vulnerability:** Raw HTML execution via untrusted AI chat responses inside Qt's Markdown renderer (`QTextDocument::setMarkdown`).
**Learning:** `QTextDocument::setMarkdown` defaults to allowing embedded HTML tags even when `QTextDocument::MarkdownDialectGitHub` is used. If this is not explicitly disabled, LLMs generating raw HTML (like `<script>alert(1)</script>` or fake login forms) will be evaluated and rendered within the desktop application.
**Prevention:** To prevent XSS or HTML injection when rendering untrusted markdown in Qt, ALWAYS apply the QTextDocument::MarkdownNoHTML feature flag by combining it with the desired dialect using the bitwise OR operator (e.g., QTextDocument::MarkdownDialectGitHub | QTextDocument::MarkdownNoHTML).
>>>>>>> main
