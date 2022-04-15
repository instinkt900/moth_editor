#pragma once

class IEditorAction {
public:
    virtual ~IEditorAction() = default;

    virtual void Do() = 0;
    virtual void Undo() = 0;

    virtual void OnImGui() = 0;
};


template <typename T>
class ChangeValueAction : public IEditorAction {
public:
    ChangeValueAction(T& valueRef, T oldValue, T newValue, std::function<void()> const& postAction)
        : m_valueRef(valueRef)
        , m_oldValue(oldValue)
        , m_newValue(newValue)
        , m_postAction(postAction) {
    }

    void Do() override {
        m_valueRef = m_newValue;
        if (m_postAction != nullptr) {
            m_postAction();
        }
    }

    void Undo() override {
        m_valueRef = m_oldValue;
        if (m_postAction != nullptr) {
            m_postAction();
        }
    }

    void OnImGui() override {
        if (ImGui::CollapsingHeader("ChangeValueAction")) {
        }
    }

    T& m_valueRef;
    T m_oldValue;
    T m_newValue;
    std::function<void()> m_postAction;
};

class BasicAction : public IEditorAction {
public:
    BasicAction(std::function<void()> doAction, std::function<void()> const& undoAction)
        : m_doAction(doAction)
        , m_undoAction(undoAction) {}

    void Do() override {
        m_doAction();
    }

    void Undo() override {
        m_undoAction();
    }

    void OnImGui() override {
        if (ImGui::CollapsingHeader("BasicAction")) {
        }
    }

private:
    std::function<void()> m_doAction;
    std::function<void()> m_undoAction;
};

template <typename T>
std::unique_ptr<IEditorAction> MakeChangeValueAction(T& valueRef, T oldValue, T newValue, std::function<void()> const& postAction) {
    return std::unique_ptr<IEditorAction>(new ChangeValueAction(valueRef, oldValue, newValue, postAction));
}

std::unique_ptr<IEditorAction> MakeVisibilityAction(std::shared_ptr<moth_ui::Node> node, bool visible);
std::unique_ptr<IEditorAction> MakeShowBoundsAction(std::shared_ptr<moth_ui::Node> node, bool visible);
