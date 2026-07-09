## 2023-10-27 - [Security Vulnerability] Unsanitized Markdown Rendering of LLM Output
**Vulnerability:** XSS/HTML Injection in QTextBrowser via Markdown
**Learning:** `QTextBrowser::setMarkdown` enables HTML rendering by default. Thus, when passing untrusted output to it, any embedded HTML tags are rendered, which can lead to XSS/Script Execution in Qt.
**Prevention:** Configure the underlying `QTextDocument` directly by passing `QTextDocument::MarkdownNoHTML` to `QTextDocument::setMarkdown()`. This disables rendering raw HTML while keeping Markdown parsing active.
