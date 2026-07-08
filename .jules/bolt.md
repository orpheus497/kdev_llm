# 2026-07-03 - SSE Token Emission UI Bottleneck

**Learning:** Emitting a signal (`chatTokenReceived`) for every individual token or line parsed within a single `QNetworkReply::readyRead` event loop causes massive O(N^2) overhead if the UI re-renders markdown upon every token. This blocks the main UI thread severely during fast LLM streaming responses.
**Action:** Always batch stream tokens parsed within a single `while (reply->canReadLine())` loop into a local `QString` buffer, and emit the combined string once at the end of the `readyRead` block to preserve stream fluidity while slashing rendering CPU cycles.
