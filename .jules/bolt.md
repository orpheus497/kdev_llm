## 2024-07-10 - Initial Setup
**Learning:** Just starting
**Action:** Will read and update

## 2024-10-27 - Batching SSE Stream Tokens
**Learning:** When processing Server-Sent Events (SSE) over Qt network loops (e.g., `QNetworkReply::readyRead`), emitting tokens line by line can cause O(N^2) markdown re-rendering slowdowns during fast network bursts. Batch incoming stream tokens into a buffer before emitting them to the UI thread.
**Action:** Always accumulate text chunks within `canReadLine()` loops and emit them once per network event.

## 2024-11-20 - Fast String Extension Matching in Qt

**Learning:** Checking for many different file extensions in a hot loop using chained `QString::endsWith(QStringLiteral(...))` calls is slow (O(N) comparisons).
**Action:** Use a `static const QSet<QStringView>` with literal `u"ext"` initializers, then extract the suffix using `path.lastIndexOf('.')` and `QStringView(path).mid(dotIndex + 1)` for an O(1) hash map lookup.

## 2024-11-20 - Fast String Moves in Qt

**Learning:** Appending locally scoped strings (like `QString`) to a container without using `std::move` causes unnecessary atomic reference count bumps on the string's internal buffer.
**Action:** When a local `QString` or similar copy-on-write object is no longer needed after being appended to a container (e.g., `QStringList::append()`), wrap it in `std::move()` to transfer ownership efficiently.

## 2024-11-20 - Cache QTextDocument creations
**Learning:** Instantiating `QTextDocument`s repeatedly in a Qt Model/Delegate during paint and sizeHint methods can be slow for large lists. Using a `QCache<QString, QTextDocument>` and storing the layout widths correctly bounds memory usage while providing massive speedups on re-renders, especially when markdown parsing is involved.
**Action:** When a UI Delegate is forced to do heavy work during rendering (like parsing and laying out markdown), wrap that logic in a bounded cache so it isn't repeated constantly.
