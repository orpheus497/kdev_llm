#pragma once
#include <QUrl>
#include "iproject.h"
namespace KDevelop {
    class IProjectController {
    public:
        virtual ~IProjectController() = default;
        virtual IProject* findProjectForUrl(const QUrl& url) const = 0;
    };
}
