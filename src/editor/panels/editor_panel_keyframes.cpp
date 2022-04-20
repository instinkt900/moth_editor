#include "common.h"
#include "editor_panel_keyframes.h"
#include "../keyframe_context.h"
#include "../utils.h"
#include "../actions/move_keyframe_action.h"
#include "../actions/modify_keyframe_action.h"
#include "../editor_layer.h"
#include "editor_panel_animation.h"

EditorPanelKeyframes::EditorPanelKeyframes(EditorLayer& editorLayer, EditorPanelAnimation& animationPanel)
    : EditorPanel(editorLayer, "Keyframe", false, false)
    , m_selectedKeyframes(animationPanel.GetSelectedKeyframes()) {
}

void EditorPanelKeyframes::Update(uint32_t ticks) {
    m_visible = !m_selectedKeyframes.empty();
}

void EditorPanelKeyframes::DrawContents() {
    for (auto&& context : m_selectedKeyframes) {
        DrawKeyframeValues(context);
    }
}

void EditorPanelKeyframes::OnClose() {
    m_selectedKeyframes.clear();
}

void EditorPanelKeyframes::DrawKeyframeValues(KeyframeContext& context) {
    ImGui::PushID(&context);
    ImGui::SetNextItemOpen(true);
    std::string const title = fmt::format("{} {} {}", GetEntityLabel(context.entity), magic_enum::enum_name(context.target), context.frameNo);
    if (ImGui::CollapsingHeader(title.c_str())) {
        assert(context.current);
        ImGui::Indent();
        auto& editableKeyframe = context.tempEditable.has_value() ? context.tempEditable.value() : *context.current;
        imgui_ext::FocusGroupBegin(&context.focusContext);
        imgui_ext::FocusGroupInputInt(
            "Frame", editableKeyframe.m_frame,
            [&](int newFrame) { BeginEdit(context); context.tempEditable.value().m_frame = newFrame; },
            [&]() { EndEdit(context); });
        imgui_ext::FocusGroupInputKeyframeValue(
            "Value", editableKeyframe.m_value,
            [&](moth_ui::KeyframeValue newValue) { BeginEdit(context); context.tempEditable.value().m_value = newValue; },
            [&]() { EndEdit(context); });
        imgui_ext::FocusGroupInputInterpType(
            "Interp", editableKeyframe.m_interpType,
            [&](moth_ui::InterpType newInterp) { BeginEdit(context); context.tempEditable.value().m_interpType = newInterp; EndEdit(context); },
            [&]() {});
        imgui_ext::FocusGroupEnd();
        ImGui::Unindent();
    }
    ImGui::PopID();
}

void EditorPanelKeyframes::BeginEdit(KeyframeContext& context) {
    if (m_originalKeyframe.has_value()) {
        return;
    }
    m_originalKeyframe = *context.current;
    context.tempEditable = *context.current;
}

void EditorPanelKeyframes::EndEdit(KeyframeContext& context) {
    if (!m_originalKeyframe.has_value()) {
        return;
    }

    auto const& originalKeyframe = m_originalKeyframe.value();
    auto const& newKeyframe = context.tempEditable.value();

    // did the frame change?
    if (originalKeyframe.m_frame != newKeyframe.m_frame) {
        // check for an existing value
        auto const& track = context.entity->m_tracks.at(context.target);
        auto const existingFrame = track->GetKeyframe(newKeyframe.m_frame);
        std::optional<moth_ui::Keyframe> replacedKeyframe;
        if (existingFrame) {
            replacedKeyframe = *existingFrame;
            track->DeleteKeyframe(existingFrame);
        }
        context.current->m_frame = newKeyframe.m_frame;
        track->SortKeyframes();

        auto moveAction = std::make_unique<MoveKeyframeAction>(context.entity, context.target, originalKeyframe.m_frame, newKeyframe.m_frame, replacedKeyframe);
        m_editorLayer.AddEditAction(std::move(moveAction));
    }

    // did the value or interp change?
    if (originalKeyframe.m_value != newKeyframe.m_value || originalKeyframe.m_interpType != newKeyframe.m_interpType) {
        context.current->m_value = newKeyframe.m_value;
        context.current->m_interpType = newKeyframe.m_interpType;
        auto modifyAction = std::make_unique<ModifyKeyframeAction>(context.entity, context.target, originalKeyframe.m_frame, originalKeyframe.m_value, newKeyframe.m_value, originalKeyframe.m_interpType, newKeyframe.m_interpType);
        m_editorLayer.AddEditAction(std::move(modifyAction));
    }

    context.tempEditable.reset();
    m_originalKeyframe.reset();
}
