// ##Script function and purpose: Implements unit tests for LlamaClient::requestCompletion using QtTest and QTcpServer to mock the HTTP endpoint.
#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include "LlamaClient.h"

// ##Class purpose: Unit test class for LlamaClient.
class TestLlamaClient : public QObject
{
    Q_OBJECT

private slots:
    // ##Method purpose: Setup before each test function.
    void init()
    {
        m_server = new QTcpServer(this);
        QVERIFY(m_server->listen(QHostAddress(QHostAddress::LocalHost)));
        m_client = new LlamaClient(this);
        m_client->setEndpointUrl(QStringLiteral("http://127.0.0.1:%1").arg(m_server->serverPort()));
    }

    // ##Method purpose: Teardown after each test function.
    void cleanup()
    {
        delete m_client;
        delete m_server;
    }

    // ##Method purpose: Helper to wait for a pending connection.
    QTcpSocket* waitForConnection() {
        // ##Loop purpose: Wait up to 1 second for a pending network connection to arrive.
        for (int i = 0; i < 10; ++i) {
            // ##Condition purpose: Check if a connection is available to accept.
            if (m_server->hasPendingConnections()) {
                return m_server->nextPendingConnection();
            }
            QTest::qWait(100);
        }
        return nullptr;
    }

    // ##Method purpose: Helper to wait for a full HTTP request.
    QByteArray waitForFullRequest(QTcpSocket* socket) {
        QByteArray requestData;
        // ##Loop purpose: Accumulate network data until a complete HTTP request (headers + body) is received.
        for (int i = 0; i < 20; ++i) {
            // ##Condition purpose: Append any available bytes from the socket.
            if (socket->bytesAvailable() > 0) {
                requestData.append(socket->readAll());
            }
            int bodyIndex = requestData.indexOf("\r\n\r\n");
            // ##Condition purpose: Parse Content-Length once headers are fully received.
            if (bodyIndex != -1) {
                int contentLength = 0;
                int clIndex = requestData.indexOf("Content-Length:");
                // ##Condition purpose: Extract the Content-Length value if present.
                if (clIndex != -1 && clIndex < bodyIndex) {
                    int valStart = clIndex + 15;
                    int valEnd = requestData.indexOf("\r\n", valStart);
                    // ##Condition purpose: Parse the integer value of Content-Length.
                    if (valEnd != -1) {
                        contentLength = requestData.mid(valStart, valEnd - valStart).trimmed().toInt();
                    }
                }
                // ##Condition purpose: Verify if the entire body has been downloaded.
                if (requestData.size() >= bodyIndex + 4 + contentLength) {
                    break;
                }
            }
            QTest::qWait(100);
        }
        return requestData;
    }

    // ##Method purpose: Verifies that requestCompletion correctly formats the HTTP request and JSON payload.
    void testRequestCompletionPayload()
    {
        const QString prefix = QStringLiteral("int main() {\n");
        const QString suffix = QStringLiteral("\n}");

        // Setup a spy to catch the request before it completes
        QSignalSpy spyCompletion(m_client, &LlamaClient::completionReceived);

        // Initiate the request
        m_client->requestCompletion(prefix, suffix);

        // Process events to let QNetworkAccessManager do its work
        QCoreApplication::processEvents();

        // Wait for connection to our mock server
        QTcpSocket *socket = waitForConnection();
        QVERIFY(socket != nullptr);

        // Wait for the full HTTP request to arrive
        QByteArray requestData = waitForFullRequest(socket);
        QVERIFY(!requestData.isEmpty());

        // ##Step purpose: Verify HTTP POST line and headers
        QVERIFY(requestData.startsWith("POST /completion HTTP/1.1\r\n"));
        QVERIFY(requestData.contains("Content-Type: application/json\r\n"));

        // ##Step purpose: Verify JSON payload
        int bodyIndex = requestData.indexOf("\r\n\r\n");
        QVERIFY(bodyIndex != -1);
        QByteArray bodyData = requestData.mid(bodyIndex + 4);

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(bodyData, &parseError);
        QCOMPARE(parseError.error, QJsonParseError::NoError);
        QVERIFY(doc.isObject());

        QJsonObject json = doc.object();
        QCOMPARE(json[QStringLiteral("prompt")].toString(), prefix);
        QCOMPARE(json[QStringLiteral("suffix")].toString(), suffix);
        QCOMPARE(json[QStringLiteral("n_predict")].toInt(), 128);
        QCOMPARE(json[QStringLiteral("temperature")].toDouble(), 0.2);
        QVERIFY(json[QStringLiteral("stream")].toBool());

        // Let's send a fake response to finish the request nicely.
        QByteArray response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/event-stream\r\n"
                              "Connection: keep-alive\r\n\r\n"
                              "data: {\"content\": \"    return 0;\"}\n"
                              "data: [DONE]\n";
        socket->write(response);
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->close();

        QVERIFY(spyCompletion.wait(1000));
        QCOMPARE(spyCompletion.count(), 1);
        QCOMPARE(spyCompletion.at(0).at(0).toString(), QStringLiteral("    return 0;"));
    }

    // ##Method purpose: Verifies that requestCompletion correctly handles an empty prompt and suffix (edge case).
    void testRequestCompletionEmptyInput()
    {
        const QString prefix = QStringLiteral("");
        const QString suffix = QStringLiteral("");

        // Setup a spy
        QSignalSpy spyCompletion(m_client, &LlamaClient::completionReceived);

        // Initiate the request
        m_client->requestCompletion(prefix, suffix);
        QCoreApplication::processEvents();

        // Wait for connection to our mock server
        QTcpSocket *socket = waitForConnection();
        QVERIFY(socket != nullptr);

        // Wait for the full HTTP request to arrive
        QByteArray requestData = waitForFullRequest(socket);
        QVERIFY(!requestData.isEmpty());

        int bodyIndex = requestData.indexOf("\r\n\r\n");
        QVERIFY(bodyIndex != -1);
        QByteArray bodyData = requestData.mid(bodyIndex + 4);

        QJsonDocument doc = QJsonDocument::fromJson(bodyData);
        QVERIFY(doc.isObject());
        QJsonObject json = doc.object();
        QCOMPARE(json[QStringLiteral("prompt")].toString(), prefix);
        QCOMPARE(json[QStringLiteral("suffix")].toString(), suffix);

        // Let's send a simple response to finish it
        QByteArray response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/event-stream\r\n\r\n"
                              "data: {\"content\": \"empty response\"}\n"
                              "data: [DONE]\n";
        socket->write(response);
        socket->flush();
        socket->waitForBytesWritten(1000);
        socket->close();

        QVERIFY(spyCompletion.wait(1000));
        QCOMPARE(spyCompletion.count(), 1);
        QCOMPARE(spyCompletion.at(0).at(0).toString(), QStringLiteral("empty response"));
    }

    // ##Method purpose: Verifies that requestCompletion handles network errors appropriately.
    void testRequestCompletionNetworkError()
    {
        // Setup a spy to catch the error signal
        QSignalSpy spyError(m_client, &LlamaClient::errorOccurred);

        // Close the server to simulate a connection refused error
        m_server->close();

        m_client->requestCompletion(QStringLiteral("test"), QStringLiteral("test"));

        // Wait for the error signal to be emitted
        QVERIFY(spyError.wait(2000));
        QCOMPARE(spyError.count(), 1);
        // The exact error message depends on Qt version, but it should be emitted
    }

private:
    QTcpServer *m_server = nullptr;
    LlamaClient *m_client = nullptr;
};

QTEST_MAIN(TestLlamaClient)
#include "TestLlamaClient.moc"
