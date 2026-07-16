#pragma once
#include <interfaces/idocument.h>
namespace KDevelop {
    class IDocumentController {
    public:
        IDocument* activeDocument() { return nullptr; }
    };
}
