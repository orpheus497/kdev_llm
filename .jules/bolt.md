## 2026-07-15 - Optimize string concatenation using QStringBuilder

**Learning:** Qt provides a `QStringBuilder` template class via `#include <QStringBuilder>` that allows using the `%` operator to combine string fragments without allocating temporary variables at each concatenation step. In heavily executed formatting functions, `prompt.reserve(...)` combined with `%` avoids O(n^2) or heavy allocation patterns during large document construction.
**Action:** When appending strings multiple times in loops or across large blocks, pre-allocate space via `.reserve()` and use the `%` operator to reduce memory copying and allocation overhead.
## 2026-07-15 - Optimize string concatenation using QStringBuilder

**Learning:** Unconditionally reserving large blocks of memory (e.g., 64KB for a prompt builder) can be highly inefficient when the full string is only conditionally constructed. Qt's `QStringBuilder` evaluates entire `%` concatenation expressions into correctly sized pre-allocations automatically.
**Action:** Move `reserve()` calls into the condition blocks where string components are actually appended, and trust `QStringBuilder` with `%` operators for inline concatenations.
