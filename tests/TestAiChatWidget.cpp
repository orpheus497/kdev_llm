// ##Script function and purpose: Implements tests for the AiChatWidget class.
#include "TestAiChatWidget.h"
#include "../src/ui/AiChatWidget.h"
#include "../src/ui/AiChatInputWidget.h"
#include <QJsonObject>
#include <QApplication>
#include <QTextBrowser>
#include <QTextDocument>
#include <QMessageBox>
#include <iostream>

TestAiChatWidget::TestAiChatWidget(QObject *parent) : QObject(parent) {}

int TestAiChatWidget::runTests() {
    int failed = 0;

    std::cout << "Running testSendMessageEmpty...\n";
    if (!testSendMessageEmpty()) {
        std::cerr << "testSendMessageEmpty FAILED\n";
        failed++;
    } else {
        std::cout << "testSendMessageEmpty PASSED\n";
    }

    std::cout << "Running testSendMessageFirstMessage...\n";
    if (!testSendMessageFirstMessage()) {
        std::cerr << "testSendMessageFirstMessage FAILED\n";
        failed++;
    } else {
        std::cout << "testSendMessageFirstMessage PASSED\n";
    }

    std::cout << "Running testSendMessageSubsequentMessage...\n";
    if (!testSendMessageSubsequentMessage()) {
        std::cerr << "testSendMessageSubsequentMessage FAILED\n";
        failed++;
    } else {
        std::cout << "testSendMessageSubsequentMessage PASSED\n";
    }

    std::cout << "Running testMarkdownSecurity...\n";
    if (!testMarkdownSecurity()) {
        std::cerr << "testMarkdownSecurity FAILED\n";
        failed++;
    } else {
        std::cout << "testMarkdownSecurity PASSED\n";
    }

    // ##Action purpose: Execute the file context aggregation tests.
    std::cout << "Running testFileContextAggregation...\n";
    // ##Condition purpose: Report failure if the file context aggregation test fails.
    if (!testFileContextAggregation()) {
        std::cerr << "testFileContextAggregation FAILED\n";
        failed++;
    } else {
        std::cout << "testFileContextAggregation PASSED\n";
    }

    std::cout << "Running testDeleteConversation...\n";
    if (!testDeleteConversation()) {
        std::cerr << "testDeleteConversation FAILED\n";
        failed++;
    } else {
        std::cout << "testDeleteConversation PASSED\n";
    }

    if (failed == 0) {
        std::cout << "All tests passed!\n";
    } else {
        std::cerr << failed << " tests failed!\n";
    }

    return failed;
}

bool TestAiChatWidget::testSendMessageEmpty() {
    AiChatWidget widget;
    // initial state after constructor calling clearChat()
    int initialHistorySize = widget.m_messageHistory.size();

    // Trigger sendMessage via private method
    widget.sendMessage(QStringLiteral(""));

    if (widget.m_messageHistory.size() != initialHistorySize) {
        return false;
    }
    return true;
}

bool TestAiChatWidget::testSendMessageFirstMessage() {
    AiChatWidget widget;
    // Assuming clearChat clears m_messageHistory to empty.

    widget.sendMessage(QStringLiteral("Hello AI"));

    // History should have System prompt + User message
    if (widget.m_messageHistory.size() != 2) {
        std::cerr << "Expected size 2, got " << widget.m_messageHistory.size() << "\n";
        return false;
    }

    QJsonObject sysMsg = widget.m_messageHistory.at(0).toObject();
    if (sysMsg[QStringLiteral("role")].toString() != QStringLiteral("system")) {
        std::cerr << "First message role not system\n";
        return false;
    }

    QJsonObject userMsg = widget.m_messageHistory.at(1).toObject();
    if (userMsg[QStringLiteral("role")].toString() != QStringLiteral("user")) {
        std::cerr << "Second message role not user\n";
        return false;
    }
    if (userMsg[QStringLiteral("content")].toString() != QStringLiteral("Hello AI")) {
        std::cerr << "Second message content mismatch\n";
        return false;
    }

    return true;
}

bool TestAiChatWidget::testSendMessageSubsequentMessage() {
    AiChatWidget widget;

    // Send first message
    widget.sendMessage(QStringLiteral("First"));

    // Send second message
    widget.sendMessage(QStringLiteral("Second"));

    // Expected size: 1 (system) + 1 (user first) + 1 (user second) = 3 messages in history
    // But since LlamaClient finishes response and adds assistant, we mock that behavior?
    // Actually we only test sendMessage which adds user messages.
    // It doesn't add assistant responses unless onChatFinished is called.

    if (widget.m_messageHistory.size() != 3) {
        std::cerr << "Expected size 3, got " << widget.m_messageHistory.size() << "\n";
        return false;
    }

    QJsonObject sysMsg = widget.m_messageHistory.at(0).toObject();
    if (sysMsg[QStringLiteral("role")].toString() != QStringLiteral("system")) {
        return false;
    }

    QJsonObject userMsg1 = widget.m_messageHistory.at(1).toObject();
    if (userMsg1[QStringLiteral("role")].toString() != QStringLiteral("user") || userMsg1[QStringLiteral("content")].toString() != QStringLiteral("First")) {
        return false;
    }

    QJsonObject userMsg2 = widget.m_messageHistory.at(2).toObject();
    if (userMsg2[QStringLiteral("role")].toString() != QStringLiteral("user") || userMsg2[QStringLiteral("content")].toString() != QStringLiteral("Second")) {
        return false;
    }

    return true;
}

bool TestAiChatWidget::testMarkdownSecurity() {
    QTextBrowser browser;
    QString markdown = QStringLiteral("Here is some HTML: <b>bold</b> <script>alert(1)</script>");

    QTextDocument::MarkdownFeatures features = QTextDocument::MarkdownDialectGitHub;
    features |= QTextDocument::MarkdownNoHTML;

    browser.document()->setMarkdown(markdown, features);

    QString html = browser.toHtml();

    if (html.contains(QStringLiteral("<script>"))) {
        std::cerr << "Test failed: HTML script tag was not escaped!" << std::endl;
        std::cerr << html.toStdString() << std::endl;
        return false;
    }

    if (html.contains(QStringLiteral("<b>"))) {
        std::cerr << "Test failed: HTML b tag was not escaped!" << std::endl;
        std::cerr << html.toStdString() << std::endl;
        return false;
    }

    if (html.contains(QStringLiteral("&lt;script&gt;")) && html.contains(QStringLiteral("&lt;b&gt;"))) {
        return true;
    } else {
        std::cerr << "Test failed: Did not find escaped HTML output." << std::endl;
        std::cerr << html.toStdString() << std::endl;
        return false;
    }
}

// ##Method purpose: Tests that file references from previous messages are retained in the system prompt without duplicates.
bool TestAiChatWidget::testFileContextAggregation() {
    AiChatWidget widget;

    // ##Action purpose: Send the first message with a file reference.
    widget.sendMessage(QStringLiteral("Review @src/main.cpp"));

    // Setup some dummy state to mimic a loaded conversation
    QJsonObject firstUserMsg;
    firstUserMsg[QStringLiteral("role")] = QStringLiteral("user");
    firstUserMsg[QStringLiteral("content")] = QStringLiteral("Review @src/main.cpp");
    widget.m_messageHistory.append(firstUserMsg);

    // ##Action purpose: Send the second message with a different file reference.
    widget.sendMessage(QStringLiteral("Now check @src/utils.cpp"));

    // And append to history
    QJsonObject secondUserMsg;
    secondUserMsg[QStringLiteral("role")] = QStringLiteral("user");
    secondUserMsg[QStringLiteral("content")] = QStringLiteral("Now check @src/utils.cpp");
    widget.m_messageHistory.append(secondUserMsg);
    // ##Action purpose: Send the third message repeating the first file reference.
    widget.sendMessage(QStringLiteral("Let's look at @src/main.cpp again"));

    // ##Condition purpose: Extract the system prompt to verify all contexts exist exactly once.
    if (widget.m_messageHistory.isEmpty()) {
        std::cerr << "Message history is empty!\n";
        return false;
    }

    QJsonObject sysMsg = widget.m_messageHistory.first().toObject();
    // ##Condition purpose: Ensure the first message in the history is a system prompt.
    if (sysMsg[QStringLiteral("role")].toString() != QStringLiteral("system")) {
        std::cerr << "First message is not system prompt!\n";
        return false;
    }

    QString content = sysMsg[QStringLiteral("content")].toString();

    // ##Condition purpose: Both files should be in the context block.
    if (!content.contains(QStringLiteral("--- Referenced File Context (@src/main.cpp) ---"))) {
        std::cerr << "Missing context for src/main.cpp\n";
        return false;
    }

    if (!content.contains(QStringLiteral("--- Referenced File Context (@src/utils.cpp) ---"))) {
        std::cerr << "Missing context for src/utils.cpp\n";
        return false;
    }

    // ##Condition purpose: Check for duplicates for src/main.cpp.
    int firstOccurrence = content.indexOf(QStringLiteral("--- Referenced File Context (@src/main.cpp) ---"));
    int lastOccurrence = content.lastIndexOf(QStringLiteral("--- Referenced File Context (@src/main.cpp) ---"));

    if (firstOccurrence != lastOccurrence) {
        std::cerr << "Duplicate context found for src/main.cpp\n";
        return false;
    }

    // ##Condition purpose: Check for duplicates for src/utils.cpp.
    int firstOccurrenceUtils = content.indexOf(QStringLiteral("--- Referenced File Context (@src/utils.cpp) ---"));
    int lastOccurrenceUtils = content.lastIndexOf(QStringLiteral("--- Referenced File Context (@src/utils.cpp) ---"));

    if (firstOccurrenceUtils != lastOccurrenceUtils) {
        std::cerr << "Duplicate context found for src/utils.cpp\n";
        return false;
    }

    return true;
}

// ##Class purpose: Subclass of AiChatWidget that mocks UI dialog interaction for unit testing.
class TestableAiChatWidget : public AiChatWidget {
public:
    using AiChatWidget::AiChatWidget;

protected:
    // ##Method purpose: Automatically confirm conversation deletion instead of showing dialog.
    QMessageBox::StandardButton askDeletionConfirmation() override {
        return QMessageBox::Yes;
    }
};

// ##Method purpose: Tests deleting a conversation and ensuring UI/model resets properly.
bool TestAiChatWidget::testDeleteConversation() {
    TestableAiChatWidget widget;

    // ##Step purpose: Simulate sending a message to create/insert a conversation in database.
    widget.sendMessage(QStringLiteral("Hello AI"));

    // ##Step purpose: Refresh list to make sure combo box is populated.
    widget.refreshConversationList();

    // ##Step purpose: Check that we have a valid conversation selected.
    int idx = widget.m_conversationSelector->currentIndex();
    qint64 convId = widget.m_conversationSelector->itemData(idx).toLongLong();
    // ##Condition purpose: Report failure if no conversation was created/selected.
    if (convId <= 0) {
        std::cerr << "No conversation was created/selected.\n";
        return false;
    }

    // ##Condition purpose: Verify delete button is enabled for valid conversation.
    if (!widget.m_deleteBtn->isEnabled()) {
        std::cerr << "Delete button was not enabled for a valid conversation.\n";
        return false;
    }

    // ##Step purpose: Trigger deleteCurrentConversation.
    widget.deleteCurrentConversation();

    // ##Condition purpose: Verify that the conversation is no longer active (reset to -1).
    if (widget.m_currentConversationId != -1) {
        std::cerr << "Current conversation ID was not reset to -1.\n";
        return false;
    }

    // ##Condition purpose: Verify delete button is now disabled (no active conversation selection).
    if (widget.m_deleteBtn->isEnabled()) {
        std::cerr << "Delete button remained enabled after deletion.\n";
        return false;
    }

    return true;
}

// ##Function purpose: Application entry point for running the standalone test suite.
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestAiChatWidget test;
    return test.runTests();
}
