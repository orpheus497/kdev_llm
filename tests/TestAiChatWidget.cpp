#include <QApplication>
#include <QTextBrowser>
#include <QTextDocument>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <iostream>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QTextBrowser browser;
    QString markdown = "Here is some HTML: <b>bold</b> <script>alert(1)</script>";

    QTextDocument::MarkdownFeatures features = QTextDocument::MarkdownDialectGitHub;
    features |= QTextDocument::MarkdownNoHTML;

    browser.document()->setMarkdown(markdown, features);

    QString html = browser.toHtml();

    if (html.contains("<script>")) {
        std::cerr << "Test failed: HTML script tag was not escaped!" << std::endl;
        std::cerr << html.toStdString() << std::endl;
        return 1;
    }

    if (html.contains("<b>")) {
        std::cerr << "Test failed: HTML b tag was not escaped!" << std::endl;
        std::cerr << html.toStdString() << std::endl;
        return 1;
    }

    if (html.contains("&lt;script&gt;") && html.contains("&lt;b&gt;")) {
        std::cout << "Test passed! Malicious HTML was safely escaped." << std::endl;
        return 0;
    } else {
        std::cerr << "Test failed: Did not find escaped HTML output." << std::endl;
        std::cerr << html.toStdString() << std::endl;
        return 1;
    }
}
