#pragma once

#include "moth_ui/widget.h"
#include "moth_ui/events/event_mouse.h"
#include "moth_ui/events/event_animation.h"

class UIButton : public moth_ui::Widget<UIButton> {
public:
    inline static char const* ClassName = "button";

    enum class State {
        Idle,
        Hovered,
        Pressed,
        Activated,
    };

    UIButton(moth_ui::Context& context, std::shared_ptr<moth_ui::LayoutEntityGroup> entity);
    virtual ~UIButton() = default;

    void Update(uint32_t ticks) override;
    bool OnEvent(moth_ui::Event const& event) override;
    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnAnimationEvent(moth_ui::EventAnimation const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
    bool OnMouseMove(moth_ui::EventMouseMove const& event);

    using ClickAction = std::function<void()>;
    void SetClickAction(ClickAction const& action);

private:
    State m_state = State::Idle;
    ClickAction m_clickAction;

    void SetState(State state);
};
