#include "ui_button.h"

#include "moth_ui/event_dispatch.h"

UIButton::UIButton(moth_ui::Context& context, std::shared_ptr<moth_ui::LayoutEntityGroup> entity)
    : Widget<UIButton>(context, entity) {}

void UIButton::Update(uint32_t ticks) {
    Group::Update(ticks);
}

bool UIButton::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &UIButton::OnAnimationEvent);
    dispatch.Dispatch(this, &UIButton::OnMouseDown);
    dispatch.Dispatch(this, &UIButton::OnMouseUp);
    dispatch.Dispatch(this, &UIButton::OnMouseMove);
    return dispatch.GetHandled() || Group::OnEvent(event);
}

bool UIButton::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (IsInBounds(event.GetPosition())) {
        SetState(State::Pressed);
        return true;
    }
    return false;
}

bool UIButton::OnAnimationEvent(moth_ui::EventAnimation const& event) {
    return false;
}

bool UIButton::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (IsInBounds(event.GetPosition()) && m_state == State::Pressed) {
        SetState(State::Activated);
    } else {
        SetState(State::Idle);
    }
    return false;
}

bool UIButton::OnMouseMove(moth_ui::EventMouseMove const& event) {
    if (IsInBounds(event.GetPosition())) {
        if (m_state != State::Pressed) {
            SetState(State::Hovered);
        }
    } else {
        if (m_state == State::Hovered) {
            SetState(State::Idle);
        }
    }
    return false;
}

void UIButton::SetClickAction(ClickAction const& action) {
    m_clickAction = action;
}

void UIButton::SetState(State state) {
    if (m_state == state) {
        return;
    }
    m_state = state;
    switch (m_state) {
    default:
    case State::Idle:
        SetAnimation("idle");
        break;
    case State::Hovered:
        SetAnimation("hover");
        break;
    case State::Pressed:
        SetAnimation("press");
        break;
    case State::Activated:
        SetAnimation("activate");
        if (m_clickAction) {
            m_clickAction();
        }
        break;
    }
}
