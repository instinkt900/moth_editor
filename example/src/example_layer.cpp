#include "example_layer.h"

#include "moth_ui/event_dispatch.h"
#include "moth_ui/node_factory.h"
#include "moth_ui/group.h"
#include "ui_button.h"
#include "canyon/events/event_window.h"

ExampleLayer::ExampleLayer(moth_ui::Context& context, std::filesystem::path const& layoutPath)
    : m_context(context) {
    LoadLayout(layoutPath);

    m_root->SetAnimation("ready");
    if (auto startButton = m_root->FindChild<UIButton>("button")) {
        startButton->SetClickAction([&]() {
            m_root->SetAnimation("transition_out");
        });
    }
}

bool ExampleLayer::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &ExampleLayer::OnRequestQuitEvent);
    bool handled = dispatch.GetHandled();
    if (!handled && m_root) {
        handled = m_root->SendEvent(event, moth_ui::Node::EventDirection::Down);
    }
    return handled;
}

void ExampleLayer::Update(uint32_t ticks) {
    auto rootCopy = m_root;
    if (rootCopy) {
        rootCopy->Update(ticks);
    }
}

void ExampleLayer::Draw() {
    moth_ui::IntVec2 const currentSize{ GetWidth(), GetHeight() };
    if (m_lastDrawnSize != currentSize) {
        moth_ui::IntRect displayRect;
        displayRect.topLeft = { 0, 0 };
        displayRect.bottomRight = currentSize;
        m_root->SetScreenRect(displayRect);
    }
    if (m_root) {
        m_root->Draw();
    }
    m_lastDrawnSize = currentSize;
}

void ExampleLayer::OnAddedToStack(moth_ui::LayerStack* stack) {
    Layer::OnAddedToStack(stack);

    if (m_root) {
        moth_ui::IntRect rect;
        rect.topLeft = { 0, 0 };
        rect.bottomRight = { GetWidth(), GetHeight() };
        m_root->SetScreenRect(rect);
    }
}

void ExampleLayer::OnRemovedFromStack() {
    Layer::OnRemovedFromStack();
}

void ExampleLayer::LoadLayout(std::filesystem::path const& path) {
    m_root = moth_ui::NodeFactory::Get().Create(m_context, path, GetWidth(), GetHeight());
    m_root->SetEventHandler([this](moth_ui::Node*, moth_ui::Event const& event) { return OnUIEvent(event); });
}

bool ExampleLayer::OnUIEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &ExampleLayer::OnAnimationStopped);
    return dispatch.GetHandled();
}

bool ExampleLayer::OnAnimationStopped(moth_ui::EventAnimationStopped const& event) {
    if (event.GetClipName() == "ready") {
        m_root->SetAnimation("idle");
        return true;
    } else if (event.GetClipName() == "transition_out") {
        LoadLayout("assets/layouts/demo.mothui");
        m_root->SetAnimation("transition_in");
        return true;
    }
    return false;
}

bool ExampleLayer::OnRequestQuitEvent(canyon::EventRequestQuit const& event) {
    m_layerStack->BroadcastEvent(canyon::EventQuit());
    return true;
}
