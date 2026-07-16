## 2024-07-10 - Initial Setup
**Learning:** Just starting
**Action:** Will read and update

## 2024-10-27 - Batching SSE Stream Tokens
**Learning:** When processing Server-Sent Events (SSE) over Qt network loops (e.g., `QNetworkReply::readyRead`), emitting tokens line by line can cause O(N^2) markdown re-rendering slowdowns during fast network bursts. Batch incoming stream tokens into a buffer before emitting them to the UI thread.
**Action:** Always accumulate text chunks within `canReadLine()` loops and emit them once per network event.

## 2026-07-16 - [Optimize Markdown Rendering]
**Learning:** Rendering markdown on every incoming SSE stream token blocks the UI thread and causes an O(N^2) slowdown during fast generation, especially as the text grows.
**Action:** Use a `QTimer` configured for 50ms (or equivalent 20FPS/60FPS interval) to debounce the rendering logic. Make sure `renderMarkdown()` is still called synchronously when the stream definitively completes.

## 2026-07-16 - [Optimize Markdown Rendering]
**Learning:** Rendering markdown on every incoming SSE stream token blocks the UI thread and causes an O(N^2) slowdown during fast generation, especially as the text grows.
**Action:** Use a `QTimer` configured for 50ms (or equivalent 20FPS/60FPS interval) to debounce the rendering logic. Make sure `renderMarkdown()` is still called synchronously when the stream definitively completes.
