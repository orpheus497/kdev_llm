// ##Script function and purpose: Implements tests for the AiChatWidget class.
#include "TestAiChatWidget.h"
#include "../ui/AiChatWidget.h"
#include "../ui/AiChatInputWidget.h"
#include <QJsonObject>
#include <QApplication>
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
    QString initialMarkdown = widget.m_rawMarkdown;

    // Trigger sendMessage via private method
    widget.sendMessage("");

    if (widget.m_messageHistory.size() != initialHistorySize) {
        return false;
    }
    if (widget.m_rawMarkdown != initialMarkdown) {
        return false;
    }
    return true;
}

bool TestAiChatWidget::testSendMessageFirstMessage() {
    AiChatWidget widget;
    // Assuming clearChat clears m_messageHistory to empty.

    widget.sendMessage("Hello AI");

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

    // Markdown should reflect the user text
    if (!widget.m_rawMarkdown.contains("**You:**\n\nHello AI\n\n---\n\n**AI:**\n\n")) {
        std::cerr << "Markdown mismatch\n";
        return false;
    }

    return true;
}

bool TestAiChatWidget::testSendMessageSubsequentMessage() {
    AiChatWidget widget;

    // Send first message
    widget.sendMessage("First");

    // Send second message
    widget.sendMessage("Second");

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

// ##Function purpose: Application entry point for running the standalone test suite.
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    TestAiChatWidget test;
    return test.runTests();
}
