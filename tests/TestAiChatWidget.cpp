// ##Script function and purpose: Implements tests for the AiChatWidget class.
#include "TestAiChatWidget.h"
#include "../src/ui/AiChatWidget.h"
#include "../src/ui/AiChatInputWidget.h"
#include <QJsonObject>
#include <QApplication>
#include <QTextBrowser>
#include <QTextDocument>
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

    std::cout << "Running testFileContextAggregation...\n";
    if (!testFileContextAggregation()) {
        std::cerr << "testFileContextAggregation FAILED\n";
        failed++;
    } else {
        std::cout << "testFileContextAggregation PASSED\n";
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

bool TestAiChatWidget::testFileContextAggregation() {
    AiChatWidget widget;

    // First message references a file
    widget.sendMessage(QStringLiteral("Review @src/main.cpp"));

    // Setup some dummy state to mimic a loaded conversation
    QJsonObject firstUserMsg;
    firstUserMsg[QStringLiteral("role")] = QStringLiteral("user");
    firstUserMsg[QStringLiteral("content")] = QStringLiteral("Review @src/main.cpp");
    widget.m_messageHistory.append(firstUserMsg);

    // Second message references another file
    widget.sendMessage(QStringLiteral("Now check @src/utils.cpp"));

    // And append to history
    QJsonObject secondUserMsg;
    secondUserMsg[QStringLiteral("role")] = QStringLiteral("user");
    secondUserMsg[QStringLiteral("content")] = QStringLiteral("Now check @src/utils.cpp");
    widget.m_messageHistory.append(secondUserMsg);

    // Third message references the first file again
    widget.sendMessage(QStringLiteral("Let's look at @src/main.cpp again"));

    // Extract the system prompt to verify all contexts exist exactly once
    if (widget.m_messageHistory.isEmpty()) {
        std::cerr << "Message history is empty!\n";
        return false;
    }

    QJsonObject sysMsg = widget.m_messageHistory.first().toObject();
    if (sysMsg[QStringLiteral("role")].toString() != QStringLiteral("system")) {
        std::cerr << "First message is not system prompt!\n";
        return false;
    }

    QString content = sysMsg[QStringLiteral("content")].toString();

    // Both files should be in the context block
    if (!content.contains(QStringLiteral("--- Referenced File Context (@src/main.cpp) ---"))) {
        std::cerr << "Missing context for src/main.cpp\n";
        return false;
    }

    if (!content.contains(QStringLiteral("--- Referenced File Context (@src/utils.cpp) ---"))) {
        std::cerr << "Missing context for src/utils.cpp\n";
        return false;
    }

    // Check for duplicates
    int firstOccurrence = content.indexOf(QStringLiteral("--- Referenced File Context (@src/main.cpp) ---"));
    int lastOccurrence = content.lastIndexOf(QStringLiteral("--- Referenced File Context (@src/main.cpp) ---"));

    if (firstOccurrence != lastOccurrence) {
        std::cerr << "Duplicate context found for src/main.cpp\n";
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
