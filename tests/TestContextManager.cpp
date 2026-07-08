// ##Script function and purpose: Tests for the ContextManager class.
#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDir>
#include <QUrl>
#include "../src/context/ContextManager.h"

// ##Class purpose: Test case class for ContextManager.
class TestContextManager : public QObject
{
    Q_OBJECT

private slots:
    // ##Method purpose: Tests getProjectRoot with an empty URL.
    void testEmptyUrl()
    {
        ContextManager manager;
        QCOMPARE(manager.getProjectRoot(QUrl()), QString());
    }

    // ##Method purpose: Tests getProjectRoot fallback directory scanning finding CMakeLists.txt.
    void testFallbackDirectoryScanning_CMakeLists()
    {
        ContextManager manager;
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QDir dir(tempDir.path());

        // Create CMakeLists.txt at root
        QFile cmakeFile(dir.filePath("CMakeLists.txt"));
        QVERIFY(cmakeFile.open(QIODevice::WriteOnly));
        cmakeFile.close();

        // Create a nested directory
        QVERIFY(dir.mkpath("src/deep/nested"));

        // Create a dummy file in the nested directory
        QString nestedFilePath = dir.filePath("src/deep/nested/main.cpp");
        QFile cppFile(nestedFilePath);
        QVERIFY(cppFile.open(QIODevice::WriteOnly));
        cppFile.close();

        // Test URL to the nested file
        QUrl fileUrl = QUrl::fromLocalFile(nestedFilePath);
        QString root = manager.getProjectRoot(fileUrl);

        // Should find the root directory containing CMakeLists.txt
        QCOMPARE(root, dir.absolutePath());
    }

    // ##Method purpose: Tests getProjectRoot fallback directory scanning finding .git directory.
    void testFallbackDirectoryScanning_Git()
    {
        ContextManager manager;
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QDir dir(tempDir.path());

        // Create .git directory at root
        QVERIFY(dir.mkdir(".git"));

        // Create a nested directory
        QVERIFY(dir.mkpath("include/nested"));

        // Create a dummy file in the nested directory
        QString nestedFilePath = dir.filePath("include/nested/header.h");
        QFile headerFile(nestedFilePath);
        QVERIFY(headerFile.open(QIODevice::WriteOnly));
        headerFile.close();

        // Test URL to the nested file
        QUrl fileUrl = QUrl::fromLocalFile(nestedFilePath);
        QString root = manager.getProjectRoot(fileUrl);

        // Should find the root directory containing .git
        QCOMPARE(root, dir.absolutePath());
    }

    // ##Method purpose: Tests getProjectRoot fallback directory scanning when no project root indicators are found.
    void testFallbackDirectoryScanning_NoProjectRoot()
    {
        ContextManager manager;
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QDir dir(tempDir.path());

        // Create a nested directory (but no CMakeLists.txt or .git)
        QVERIFY(dir.mkpath("some/random/dir"));

        // Create a dummy file
        QString nestedFilePath = dir.filePath("some/random/dir/file.txt");
        QFile txtFile(nestedFilePath);
        QVERIFY(txtFile.open(QIODevice::WriteOnly));
        txtFile.close();

        // Test URL to the nested file
        QUrl fileUrl = QUrl::fromLocalFile(nestedFilePath);
        QString root = manager.getProjectRoot(fileUrl);

        // Should return empty string since it couldn't find a root
        QCOMPARE(root, QString());
    }
};

QTEST_GUILESS_MAIN(TestContextManager)
#include "TestContextManager.moc"
