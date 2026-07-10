## 2026-07-10 - AiChatInputWidget UX Improvements
**Learning:** Qt accessible names (`setAccessibleName`) map directly to ARIA labels, making widgets usable by screen readers. Disabling interactive elements when unneeded (empty send button) prevents user errors and adds micro-polish. Proper focus management (`setFocus`) after clearing states prevents unnecessary clicks.
**Action:** Always map screen-reader roles via `setAccessibleName` on Qt widgets missing text, and bind interactive states (like button enabled) directly to input text-change signals for immediate feedback.
