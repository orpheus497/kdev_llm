## 2026-07-10 - AiChatInputWidget UX Improvements
**Learning:** Qt accessible names (`setAccessibleName`) map directly to ARIA labels, making widgets usable by screen readers. Disabling interactive elements when unneeded (empty send button) prevents user errors and adds micro-polish. Proper focus management (`setFocus`) after clearing states prevents unnecessary clicks.
**Action:** Always map screen-reader roles via `setAccessibleName` on Qt widgets missing text, and bind interactive states (like button enabled) directly to input text-change signals for immediate feedback.

## 2026-07-22 - Confirmation Dialogs for Destructive Actions

**Learning:** For Qt UI development in this project, destructive actions like deleting conversation history can lead to accidental data loss if triggered without warning. Using `QMessageBox::question` is a standard and effective way to implement confirmation dialogs.
**Action:** Always use `QMessageBox::question` to prompt users for confirmation before executing destructive actions (e.g., deletions) to prevent accidental data loss. Ensure appropriate localization using `i18n()`.
