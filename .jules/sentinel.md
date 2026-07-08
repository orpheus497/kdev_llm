# Sentinel Security Journal

## 2024-07-07 - QTextBrowser URL Security
**Vulnerability:** QTextBrowser was configured with `setOpenExternalLinks(true)`, which automatically passes clicked markdown links to the OS.
**Learning:** LLMs can generate malicious markdown links with arbitrary URI schemes (e.g., `file://`, `javascript:`, or OS-specific app links). If clicked by the user, this can lead to arbitrary file reads or code execution.
**Prevention:** Disable `openExternalLinks` and `openLinks`. Manually connect the `anchorClicked` signal to a custom slot that validates the URL scheme (allowing only `http` and `https`) before calling `QDesktopServices::openUrl()`.
