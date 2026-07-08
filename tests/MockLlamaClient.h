#pragma once
#include "../src/network/LlamaClient.h"

class MockLlamaClient : public LlamaClient {
    Q_OBJECT
public:
    explicit MockLlamaClient(QObject *parent = nullptr) : LlamaClient(parent), requestCompletionCalled(false) {}

    void requestCompletion(const QString &prefix, const QString &suffix) override {
        requestCompletionCalled = true;
        m_lastPrefix = prefix;
        m_lastSuffix = suffix;
    }

    bool requestCompletionCalled;
    QString m_lastPrefix;
    QString m_lastSuffix;
};
