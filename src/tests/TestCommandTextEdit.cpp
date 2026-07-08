#include <QtTest>
#include <QTextEdit>
#include "ui/AiChatInputWidget.h"

// ##Class purpose: Unit tests for CommandTextEdit's file completion parsing logic.
class TestCommandTextEdit : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCompletionUnderCursor_data();
    void testCompletionUnderCursor();
};

void TestCommandTextEdit::testCompletionUnderCursor_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("cursorPos");
    QTest::addColumn<int>("expectedType");
    QTest::addColumn<QString>("expectedPrefix");
    QTest::addColumn<QString>("expectedFilterText");
    QTest::addColumn<int>("expectedPrefixStart");

    // Empty text
    QTest::newRow("empty") << "" << 0
                           << static_cast<int>(CommandTextEdit::None) << "" << "" << -1;

    // No '@' symbol
    QTest::newRow("no_at_symbol") << "hello world" << 11
                                  << static_cast<int>(CommandTextEdit::None) << "" << "" << -1;

    // Simple '@' at the start
    QTest::newRow("at_start_empty") << "@" << 1
                                    << static_cast<int>(CommandTextEdit::File) << "@" << "" << 0;

    // '@' with text at start
    QTest::newRow("at_start_with_text") << "@file" << 5
                                        << static_cast<int>(CommandTextEdit::File) << "@file" << "file" << 0;

    // Cursor before the end of the prefix
    QTest::newRow("at_start_cursor_middle") << "@file" << 3 // Cursor after 'i'
                                            << static_cast<int>(CommandTextEdit::File) << "@fi" << "fi" << 0;

    // '@' after space
    QTest::newRow("at_after_space") << "hello @file" << 11
                                    << static_cast<int>(CommandTextEdit::File) << "@file" << "file" << 6;

    // '@' inside text (no space before)
    // Avoid triggering completion on emails like "user@domain.com"
    QTest::newRow("at_inside_text") << "hello@file" << 10
                                    << static_cast<int>(CommandTextEdit::None) << "" << "" << -1;

    // Space after '@' - cursor right after space
    QTest::newRow("space_after_at") << "@ file" << 2
                                    << static_cast<int>(CommandTextEdit::None) << "" << "" << -1;

    // Text after space after '@' - cursor at end
    QTest::newRow("text_after_space_after_at") << "@ file.txt" << 10
                                               << static_cast<int>(CommandTextEdit::None) << "" << "" << -1;

    // Multiple '@' symbols - should take the most recent one backwards
    QTest::newRow("multiple_at_symbols") << "hello @first @second" << 20
                                         << static_cast<int>(CommandTextEdit::File) << "@second" << "second" << 13;
}

void TestCommandTextEdit::testCompletionUnderCursor()
{
    QFETCH(QString, text);
    QFETCH(int, cursorPos);
    QFETCH(int, expectedType);
    QFETCH(QString, expectedPrefix);
    QFETCH(QString, expectedFilterText);
    QFETCH(int, expectedPrefixStart);

    CommandTextEdit edit;
    edit.setPlainText(text);

    QTextCursor cursor = edit.textCursor();
    cursor.setPosition(cursorPos);
    edit.setTextCursor(cursor);

    CommandTextEdit::CompletionContext ctx = edit.completionUnderCursor();

    QCOMPARE(static_cast<int>(ctx.type), expectedType);
    if (ctx.type == CommandTextEdit::File) {
        QCOMPARE(ctx.prefix, expectedPrefix);
        QCOMPARE(ctx.filterText, expectedFilterText);
        QCOMPARE(ctx.prefixStart, expectedPrefixStart);
    }
}

QTEST_MAIN(TestCommandTextEdit)
#include "TestCommandTextEdit.moc"
