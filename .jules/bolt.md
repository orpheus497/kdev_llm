## 2024-07-10 - Initial Setup
**Learning:** Just starting
**Action:** Will read and update

## 2024-10-27 - Batching SSE Stream Tokens
**Learning:** When processing Server-Sent Events (SSE) over Qt network loops (e.g., `QNetworkReply::readyRead`), emitting tokens line by line can cause O(N^2) markdown re-rendering slowdowns during fast network bursts. Batch incoming stream tokens into a buffer before emitting them to the UI thread.
**Action:** Always accumulate text chunks within `canReadLine()` loops and emit them once per network event.
## 2024-05-30 - Context Manager Project Root Caching\n**Learning:** Repeatedly scanning the directory tree for a project root synchronously on the UI thread causes severe performance degradation, especially in large, nested projects, and is unnecessary for the same document.\n**Action:** Implemented a QHash cache mapping document URLs to their discovered root paths. This avoids redundant file system I/O, reducing the time for 1000 iterations from ~240ms to ~5ms.
