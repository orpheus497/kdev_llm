#pragma once
namespace KTextEditor { class View; }
namespace KDevelop {
    class IDocument {
    public:
        KTextEditor::View* activeTextView() { return nullptr; }
    };
}
