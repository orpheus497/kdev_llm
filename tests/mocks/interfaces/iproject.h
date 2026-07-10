#pragma once
#include <QUrl>
#include <QString>
namespace KDevelop {
    class IProject {
    public:
        virtual ~IProject() = default;
        virtual QUrl path() const = 0;
        virtual QString name() const = 0;
    };
}
