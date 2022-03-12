#include "common.h"
#include "keyframe_widget.h"
#include "keyframe_context.h"
#include "moth_ui/layout/layout_entity.h"
#include "utils.h"
#include "editor_layer.h"
#include "actions/move_keyframe_action.h"
#include "moth_ui/utils/imgui_ext.h"

KeyframeWidget::KeyframeWidget(EditorLayer& editorLayer, std::vector<KeyframeContext>& selectedKeyframes)
    : m_editorLayer(editorLayer)
    , m_selectedKeyframes(selectedKeyframes) {
}

KeyframeWidget::~KeyframeWidget() {
}

void KeyframeWidget::Draw() {
    if (m_open) {
        if (ImGui::Begin("Selected Keyframes", &m_open)) {
            for (auto&& context : m_selectedKeyframes) {
                DrawKeyframe(context);
            }
        }
        ImGui::End();
    }
}

void KeyframeWidget::DrawKeyframe(KeyframeContext& context) {
    ImGui::PushID(&context);
    ImGui::SetNextItemOpen(true);
    std::string const title = fmt::format("{} {} {}", GetEntityLabel(context.entity), magic_enum::enum_name(context.target), context.frameNo);
    if (ImGui::CollapsingHeader(title.c_str())) {
        assert(context.current);
        ImGui::Indent();
        int currentFrameNo = context.current->m_frame;
        if (ImGui::InputInt("Frame", &currentFrameNo, 0)) {
            OnFrameChanged(context, currentFrameNo);
        }
        CheckLostFocus(context);
        imgui_ext::InputKeyframeValue("Value", &context.current->m_value);
        ImGui::Unindent();
    }
    ImGui::PopID();
}

void KeyframeWidget::OnFrameChanged(KeyframeContext& context, int destFrameNo) {
    if (context.frameNo == destFrameNo) {
        // no change
        return;
    }

    if (!m_currentMoveContext) {
        m_currentMoveContext = std::make_unique<MoveContext>(MoveContext{ &context, context.frameNo, nullptr });
    }

    if (context.frameNo == 0) {
        // if we're frame zero, only allow the move if theres an existing frame zero
        if (m_currentMoveContext->existingFrame == nullptr || m_currentMoveContext->existingFrame->m_frame != 0) {
            m_currentMoveContext.reset();
            return;
        }
    }

    // grab a reference to any existing frame here so we can grab it when we commit the move
    auto track = context.entity->GetAnimationTracks().at(context.target);
    m_currentMoveContext->existingFrame = track->GetKeyframe(destFrameNo);
    context.frameNo = context.current->m_frame = destFrameNo;
}

void KeyframeWidget::CheckLostFocus(KeyframeContext& context) {
    if (m_currentMoveContext) {
        if (m_currentMoveContext->context == &context) {
            if (!ImGui::IsItemFocused()) {
                OnMoveComplete();
            }
        }
    }
}

void KeyframeWidget::OnMoveComplete() {
    auto& context = *m_currentMoveContext->context;
    if (context.current->m_frame != m_currentMoveContext->originalFrameNo) {
        auto track = context.entity->GetAnimationTracks().at(context.target);
        // check for an existing value
        std::optional<moth_ui::KeyframeValue> replacedValue;
        if (m_currentMoveContext->existingFrame) {
            replacedValue = m_currentMoveContext->existingFrame->m_value;
            track->DeleteKeyframe(m_currentMoveContext->existingFrame);
            m_currentMoveContext->existingFrame = nullptr;
        }
        track->SortKeyframes();

        auto moveAction = std::make_unique<MoveKeyframeAction>(context.entity, context.target, m_currentMoveContext->originalFrameNo, context.frameNo, replacedValue);
        m_editorLayer.AddEditAction(std::move(moveAction));
    }
    m_currentMoveContext.reset();
}
