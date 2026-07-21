// ##Script function and purpose: Defines the test suite for the AiChatWidget class.
#pragma once

#include <QObject>

// ##Class purpose: Tests the behavior of the AiChatWidget's sendMessage function.
class TestAiChatWidget : public QObject {
    Q_OBJECT

public:
    // ##Method purpose: Constructor for the test class.
    explicit TestAiChatWidget(QObject *parent = nullptr);

    // ##Method purpose: Executes all tests and returns the failure count.
    int runTests();

private:
    // ##Method purpose: Tests sending an empty message does not modify state.
    static bool testSendMessageEmpty();

    // ##Method purpose: Tests the first message sent adds the system prompt and the user message.
    static bool testSendMessageFirstMessage();

    // ##Method purpose: Tests subsequent messages append to history and update system prompt if needed.
    static bool testSendMessageSubsequentMessage();

    // ##Method purpose: Tests that markdown rendering does not allow unsanitized HTML execution.
    static bool testMarkdownSecurity();

    // ##Method purpose: Tests that file references from previous messages are retained in the system prompt without duplicates.
    static bool testFileContextAggregation();
};
