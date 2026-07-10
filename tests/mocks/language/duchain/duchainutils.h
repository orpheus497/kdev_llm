#pragma once
#include <QUrl>
#include <QString>
namespace KDevelop {
    class AbstractType {
    public:
        QString toString() const { return QString(); }
    };
    class Declaration {
    public:
        QString toString() const { return QString(); }
        AbstractType* abstractType() const { return nullptr; }
    };
    namespace DUChainUtils {
        struct ItemUnderCursor {
            Declaration* declaration = nullptr;
        };
        inline ItemUnderCursor itemUnderCursor(const QUrl&, const void*) { return {}; }
    }
}
