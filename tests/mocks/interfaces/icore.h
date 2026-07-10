#pragma once
#include "iprojectcontroller.h"
#include <interfaces/idocumentcontroller.h>
namespace KDevelop {
    class ICore {
    public:
        virtual ~ICore() = default;
        virtual IProjectController* projectController() = 0;
        virtual IDocumentController* documentController() { return nullptr; }
        static ICore* self();
    };
}
