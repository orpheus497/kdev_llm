// ##Script function and purpose: Provides a mock LlamaClient object for unit testing network logic without actual HTTP requests.
#pragma once
#include "../src/network/LlamaClient.h"

// ##Class purpose: Mock for LlamaClient to intercept requestCompletion signals and store them for assertions.
class MockLlamaClient : public LlamaClient {
    Q_OBJECT
public:
    // ##Method purpose: Constructor setting up the initial request tracking state.
    explicit MockLlamaClient(QObject *parent = nullptr) : LlamaClient(parent), requestCompletionCalled(false) {}

    // ##Method purpose: Intercepts inline completion HTTP requests and records the parameters.
    void requestCompletion(const QString &prefix, const QString &suffix) override {
        requestCompletionCalled = true;
        m_lastPrefix = prefix;
        m_lastSuffix = suffix;
    }

    bool requestCompletionCalled;
    QString m_lastPrefix;
    QString m_lastSuffix;
};
