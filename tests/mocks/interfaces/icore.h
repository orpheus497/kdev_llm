#pragma once
#include "iprojectcontroller.h"
namespace KDevelop {
    class ICore {
    public:
        virtual ~ICore() = default;
        virtual IProjectController* projectController() = 0;
        static ICore* self();
    };
}
