#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "../src/context/ContextManager.h"

// Mock ICore implementation for our test
#include "interfaces/icore.h"
#include "interfaces/iprojectcontroller.h"
#include "interfaces/iproject.h"
#include <KTextEditor/Document>

class MockProject : public KDevelop::IProject {
    QUrl m_path;
public:
    MockProject(const QUrl& path) : m_path(path) {}
    QUrl path() const override { return m_path; }
    QString name() const override { return "MockProject"; }
};

class MockProjectController : public KDevelop::IProjectController {
    MockProject* m_project = nullptr;
public:
    void setProject(MockProject* proj) { m_project = proj; }
    KDevelop::IProject* findProjectForUrl(const QUrl&) const override { return m_project; }
};

class MockCore : public KDevelop::ICore {
    MockProjectController m_pc;
public:
    MockProjectController* projectController() override { return &m_pc; }
    static MockCore* s_instance;
};
MockCore* MockCore::s_instance = nullptr;

KDevelop::ICore* KDevelop::ICore::self() { return MockCore::s_instance; }

class MockDocument : public KTextEditor::Document {
    QUrl m_url;
public:
    MockDocument(const QUrl& url = QUrl()) : m_url(url) {}
    void setUrl(const QUrl& url) { m_url = url; }
    QUrl url() const override { return m_url; }
};

class TestContextManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        MockCore::s_instance = new MockCore();
    }

    void cleanupTestCase() {
        delete MockCore::s_instance;
        MockCore::s_instance = nullptr;
    }

    void testGetProjectRootNullDoc() {
        ContextManager cm;
        QCOMPARE(cm.getProjectRoot(nullptr), QString());
    }

    void testGetProjectRootEmptyUrl() {
        ContextManager cm;
        MockDocument doc;
        QCOMPARE(cm.getProjectRoot(&doc), QString());
    }

    void testGetProjectRootWithKDevelopProject() {
        ContextManager cm;
        MockDocument doc(QUrl::fromLocalFile("/path/to/some/file.cpp"));
        MockProject proj(QUrl::fromLocalFile("/path/to/project"));

        MockCore::s_instance->projectController()->setProject(&proj);

        QCOMPARE(cm.getProjectRoot(&doc), QString("/path/to/project"));
    }

    void testGetProjectRootFallbackGit() {
        ContextManager cm;
        MockCore::s_instance->projectController()->setProject(nullptr);

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create a fake git dir
        QVERIFY(QDir().mkpath(tempDir.path() + "/.git"));

        // Create a nested dir for our file
        QString nestedPath = tempDir.path() + "/src/nested";
        QVERIFY(QDir().mkpath(nestedPath));

        MockDocument doc(QUrl::fromLocalFile(nestedPath + "/file.cpp"));

        QCOMPARE(cm.getProjectRoot(&doc), tempDir.path());
    }

    void testGetProjectRootFallbackCMake() {
        ContextManager cm;
        MockCore::s_instance->projectController()->setProject(nullptr);

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // Create a CMakeLists.txt
        QFile file(tempDir.path() + "/CMakeLists.txt");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();

        // Create a nested dir for our file
        QString nestedPath = tempDir.path() + "/src/nested";
        QVERIFY(QDir().mkpath(nestedPath));

        MockDocument doc(QUrl::fromLocalFile(nestedPath + "/file.cpp"));

        QCOMPARE(cm.getProjectRoot(&doc), tempDir.path());
    }

    void testGetProjectRootNoProjectRootFound() {
        ContextManager cm;
        MockCore::s_instance->projectController()->setProject(nullptr);

        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        // No .git or CMakeLists.txt
        QString nestedPath = tempDir.path() + "/src/nested";
        QVERIFY(QDir().mkpath(nestedPath));

        MockDocument doc(QUrl::fromLocalFile(nestedPath + "/file.cpp"));

        QCOMPARE(cm.getProjectRoot(&doc), QString());
    }
};

QTEST_MAIN(TestContextManager)
#include "TestContextManager.moc"
