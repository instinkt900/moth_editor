#include "common.h"
#include "editor_panel_animation.h"
#include "../editor_layer.h"
#include "../utils.h"
#include "../actions/composite_action.h"
#include "../actions/modify_clip_action.h"
#include "../actions/move_keyframe_action.h"
#include "../actions/add_keyframe_action.h"
#include "../actions/delete_keyframe_action.h"
#include "../actions/modify_keyframe_action.h"
#include "../actions/add_clip_action.h"
#include "../actions/delete_clip_action.h"
#include "../actions/add_event_action.h"
#include "../actions/delete_event_action.h"
#include "../actions/modify_event_action.h"
#include "moth_ui/group.h"
#include "moth_ui/layout/layout_entity_group.h"

#undef min
#undef max

using namespace moth_ui;

EditorPanelAnimation::EditorPanelAnimation(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Animation", visible, true) {
}

void EditorPanelAnimation::OnLayoutLoaded() {
    ClearSelections();

    m_minFrame = 0;
    m_maxFrame = 100;
    m_currentFrame = 0;
    m_firstVisibleFrame = 0;

    m_framePixelWidth = 10.f;

    m_extraData.clear();
}

void EditorPanelAnimation::DrawContents() {
    m_currentFrame = m_editorLayer.GetSelectedFrame();
    DrawWidget();
    m_editorLayer.SetSelectedFrame(m_currentFrame);
}

char const* EditorPanelAnimation::GetChildLabel(int index) const {
    auto child = m_group->GetChildren()[index];
    static std::string stringBuffer;
    stringBuffer = fmt::format("{}: {}", index, GetEntityLabel(child->GetLayoutEntity()));
    return stringBuffer.c_str();
}

char const* EditorPanelAnimation::GetTrackLabel(AnimationTrack::Target target) const {
    static std::string temp_string;
    temp_string = magic_enum::enum_name(target);
    return temp_string.c_str();
}

void EditorPanelAnimation::ClearSelections() {
    m_selections.clear();
}

void EditorPanelAnimation::DeleteSelections() {
    if (m_selections.empty()) {
        return;
    }

    auto entity = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity());

    std::vector<std::unique_ptr<IEditorAction>> actions;
    for (auto& context : m_selections) {
        if (auto clipContext = std::get_if<ClipContext>(&context)) {
            auto action = std::make_unique<DeleteClipAction>(entity, *clipContext->clip);
            actions.push_back(std::move(action));
        } else if (auto eventContext = std::get_if<EventContext>(&context)) {
            auto action = std::make_unique<DeleteEventAction>(entity, *eventContext->event);
            actions.push_back(std::move(action));
        } else if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
            auto action = std::make_unique<DeleteKeyframeAction>(keyframeContext->entity, keyframeContext->target, keyframeContext->current->m_frame, keyframeContext->current->m_value);
            actions.push_back(std::move(action));
        }
    }

    if (actions.size() > 1) {
        auto compositeAction = std::make_unique<CompositeAction>();
        auto& targetActions = compositeAction->GetActions();
        targetActions.insert(std::end(targetActions), std::make_move_iterator(std::begin(actions)), std::make_move_iterator(std::end(actions)));
        m_editorLayer.PerformEditAction(std::move(compositeAction));
    } else if (!actions.empty()) {
        m_editorLayer.PerformEditAction(std::move(actions[0]));
    }

    ClearSelections();

    m_editorLayer.Refresh();
}

void EditorPanelAnimation::SelectClip(moth_ui::AnimationClip* clip) {
    if (!IsClipSelected(clip)) {
        ClipContext context;
        context.clip = clip;
        context.mutableValue = *clip;
        m_selections.push_back(context);
    }
}

void EditorPanelAnimation::DeselectClip(moth_ui::AnimationClip* clip) {
    for (auto it = std::begin(m_selections); it != std::end(m_selections); ++it) {
        if (auto clipSelection = std::get_if<ClipContext>(&(*it))) {
            if (clipSelection->clip == clip) {
                m_selections.erase(it);
                break;
            }
        }
    }
}

bool EditorPanelAnimation::IsClipSelected(moth_ui::AnimationClip* clip) {
    return GetSelectedClipContext(clip) != nullptr;
}

ClipContext* EditorPanelAnimation::GetSelectedClipContext(moth_ui::AnimationClip* clip) {
    for (auto& selection : m_selections) {
        if (auto clipSelection = std::get_if<ClipContext>(&selection)) {
            if (clipSelection->clip == clip) {
                return clipSelection;
            }
        }
    }
    return nullptr;
}

void EditorPanelAnimation::SelectEvent(moth_ui::AnimationEvent* event) {
    if (!IsEventSelected(event)) {
        EventContext context;
        context.event = event;
        context.mutableValue = *event;
        m_selections.push_back(context);
    }
}

void EditorPanelAnimation::DeselectEvent(moth_ui::AnimationEvent* event) {
    for (auto it = std::begin(m_selections); it != std::end(m_selections); ++it) {
        if (auto eventSelection = std::get_if<EventContext>(&(*it))) {
            if (eventSelection->event == event) {
                m_selections.erase(it);
                break;
            }
        }
    }
}

bool EditorPanelAnimation::IsEventSelected(moth_ui::AnimationEvent* event) {
    return GetSelectedEventContext(event) != nullptr;
}

EventContext* EditorPanelAnimation::GetSelectedEventContext(moth_ui::AnimationEvent* event) {
    for (auto& selection : m_selections) {
        if (auto eventSelection = std::get_if<EventContext>(&selection)) {
            if (eventSelection->event == event) {
                return eventSelection;
            }
        }
    }
    return nullptr;
}

void EditorPanelAnimation::SelectKeyframe(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo) {
    if (!IsKeyframeSelected(entity, target, frameNo)) {
        if (auto keyframe = entity->m_tracks.at(target)->GetKeyframe(frameNo)) {
            KeyframeContext context;
            context.entity = entity;
            context.target = target;
            context.mutableFrame = frameNo;
            context.current = keyframe;
            m_selections.push_back(context);
        }
    }
}

void EditorPanelAnimation::DeselectKeyframe(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo) {
    auto const it = ranges::find_if(m_selections, [&](auto context) {
        if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
            return keyframeContext->entity == entity && keyframeContext->target == target && keyframeContext->current->m_frame == frameNo;
        }
        return false;
    });
    if (std::end(m_selections) != it) {
        m_selections.erase(it);
    }
}

bool EditorPanelAnimation::IsKeyframeSelected(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo) {
    return GetSelectedKeyframeContext(entity, target, frameNo) != nullptr;
}

KeyframeContext* EditorPanelAnimation::GetSelectedKeyframeContext(std::shared_ptr<LayoutEntity> entity, AnimationTrack::Target target, int frameNo) {
    auto const it = ranges::find_if(m_selections, [&](auto context) {
        if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
            return keyframeContext->entity == entity && keyframeContext->target == target && keyframeContext->current->m_frame == frameNo;
        }
        return false;
    });

    if (std::end(m_selections) != it) {
        return std::get_if<KeyframeContext>(&(*it));
    }

    return nullptr;
}

void EditorPanelAnimation::FilterKeyframeSelections(std::shared_ptr<LayoutEntity> entity, int frameNo) {
    for (auto it = std::begin(m_selections); it != std::end(m_selections); /* skip */) {
        auto& context = *it;
        if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
            if (keyframeContext->entity != entity || keyframeContext->current->m_frame != frameNo) {
                it = m_selections.erase(it);
                continue;
            }
        }
        ++it;
    }
}

EditorPanelAnimation::RowDimensions EditorPanelAnimation::AddRow(char const* label, RowOptions const& rowOptions) {
    RowDimensions resultDimensions;

    ImVec2 const rowMin = m_panelBounds.Min + ImVec2{ 0.0f, m_rowHeight * m_rowCounter };
    ImVec2 const rowMax = rowMin + ImVec2{ m_panelBounds.GetWidth(), m_rowHeight };
    ImRect const rowBounds{ rowMin, rowMax };
    ImU32 const rowColor = rowOptions.colorOverride.has_value() ? rowOptions.colorOverride.value() : (((m_rowCounter % 2) == 0) ? 0xFF3A3636 : 0xFF413D3D);

    resultDimensions.rowBounds = rowBounds;

    // background
    m_drawList->AddRectFilled(rowMin, rowMax, rowColor, 0);

    float cursorPos = rowMin.x;

    if (rowOptions.expandable) {
        ImVec2 const arrowSize{ 20.0f, m_rowHeight };
        ImVec2 const arrowMin{ cursorPos, rowMin.y };
        ImVec2 const arrowMax = arrowMin + arrowSize;
        ImRect const arrowBounds{ arrowMin, arrowMax };
        ImGui::RenderArrow(m_drawList, arrowMin, ImGui::GetColorU32(ImGuiCol_Text), rowOptions.expanded ? ImGuiDir_Down : ImGuiDir_Right);
        cursorPos += arrowBounds.GetSize().x;
        resultDimensions.buttonBounds = arrowBounds;
    }

    // optional label
    if (label != nullptr) {
        ImVec2 const labelMin{ cursorPos, rowMin.y };
        ImVec2 const labelMax{ rowMin.x + m_legendWidth, rowMax.y };
        ImRect const labelBounds{ labelMin, labelMax };
        ImVec2 const textPos = labelMin + ImVec2(rowOptions.indented ? 50.0f : 3.0f, 0.0f);
        m_drawList->AddText(textPos, 0xFFFFFFFF, label);
        cursorPos += labelBounds.GetSize().x;
        resultDimensions.labelBounds = labelBounds;
    }

    ImVec2 const trackMin{ rowMin.x + m_legendWidth, rowMin.y };
    ImVec2 const trackMax = rowMax;
    resultDimensions.trackBounds = ImRect{ trackMin, trackMax };
    resultDimensions.trackOffset = -static_cast<int>(m_firstVisibleFrame) * m_framePixelWidth;

    ++m_rowCounter;

    return resultDimensions;
}

void EditorPanelAnimation::DrawFrameRow() {
    ImGuiIO const& io = ImGui::GetIO();
    RowDimensions const rowDimensions = AddRow(nullptr, {});

    //header frame number and lines
    int modFrameCount = 10;
    int frameStep = 1;
    while ((modFrameCount * m_framePixelWidth) < 150) {
        modFrameCount *= 2;
        frameStep *= 2;
    };
    int const halfModFrameCount = modFrameCount / 2;

    for (int i = m_minFrame; i <= m_maxFrame; i += frameStep) {
        bool const baseIndex = ((i % modFrameCount) == 0) || (i == m_maxFrame || i == m_minFrame);
        bool const halfIndex = (i % halfModFrameCount) == 0;
        float const px = rowDimensions.trackBounds.Min.x + (i - m_minFrame) * m_framePixelWidth + rowDimensions.trackOffset;
        float const tiretStart = baseIndex ? 4.0f : (halfIndex ? 10.0f : 14.0f);

        // frame edge ticks
        if (px >= rowDimensions.trackBounds.Min.x && px <= rowDimensions.trackBounds.Max.x) {
            ImVec2 const start{ px, m_panelBounds.Min.y + tiretStart };
            ImVec2 const end{ px, m_panelBounds.Min.y + m_rowHeight - 1 };
            m_drawList->AddLine(start, end, 0xFF606060, 1);

            // frame numbers
            if (baseIndex) {
                static char tmps[512];
                ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", i);
                m_drawList->AddText(ImVec2(px + 3.f, m_panelBounds.Min.y), 0xFFBBBBBB, tmps);
            }
        }
    }

    // moving current frame
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && rowDimensions.trackBounds.Contains(io.MousePos)) {
        m_movingCurrentFrame = true;
    }

    float const frameCount = static_cast<float>(ImMax(m_maxFrame - m_minFrame, 1));
    if (m_movingCurrentFrame) {
        if (frameCount) {
            m_currentFrame = static_cast<int>((io.MousePos.x - rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset) / m_framePixelWidth) + static_cast<int>(m_firstVisibleFrame);
            if (m_currentFrame < m_minFrame)
                m_currentFrame = m_minFrame;
            if (m_currentFrame >= m_maxFrame)
                m_currentFrame = m_maxFrame;
        }
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_movingCurrentFrame = false;
        }
    }
}

bool EditorPanelAnimation::DrawClipPopup() {
    if (ImGui::BeginPopup("clip_popup")) {
        auto layout = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity());
        auto const it = ranges::find_if(layout->m_clips, [&](auto const& clip) {
            return clip->m_startFrame <= m_clickedFrame && clip->m_endFrame >= m_clickedFrame;
        });

        ClipContext* clipContext = nullptr;
        if (std::end(layout->m_clips) != it) {
            clipContext = GetSelectedClipContext(it->get());
        }

        if (m_selections.empty() && ImGui::MenuItem("Add")) {
            auto entity = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity());
            auto newClip = moth_ui::AnimationClip();
            newClip.m_name = "New Clip";
            newClip.m_startFrame = m_clickedFrame;
            newClip.m_endFrame = newClip.m_startFrame + 10;
            auto action = std::make_unique<AddClipAction>(entity, newClip);
            m_editorLayer.PerformEditAction(std::move(action));
        }
        if (!m_selections.empty() && ImGui::MenuItem("Delete")) {
            DeleteSelections();
        }

        if (clipContext && ImGui::BeginMenu("Edit")) {
            if (!m_pendingClipEdit.has_value()) {
                m_pendingClipEdit = EditContext<AnimationClip>(clipContext->clip);
            }

            static char buf[1024];
            snprintf(buf, 1024, "%s", m_pendingClipEdit->mutableValue.m_name.c_str());
            if (ImGui::InputText("Name", buf, 1024)) {
                m_pendingClipEdit.value().mutableValue.m_name = std::string(buf);
            }

            ImGui::InputFloat("FPS", &m_pendingClipEdit.value().mutableValue.m_fps);

            std::string preview = std::string(magic_enum::enum_name(m_pendingClipEdit.value().mutableValue.m_loopType));
            if (ImGui::BeginCombo("Loop Type", preview.c_str())) {
                for (size_t n = 0; n < magic_enum::enum_count<AnimationClip::LoopType>(); n++) {
                    auto const enumValue = magic_enum::enum_value<AnimationClip::LoopType>(n);
                    std::string enumName = std::string(magic_enum::enum_name(enumValue));
                    const bool is_selected = m_pendingClipEdit.value().mutableValue.m_loopType == enumValue;
                    if (ImGui::Selectable(enumName.c_str(), is_selected)) {
                        m_pendingClipEdit.value().mutableValue.m_loopType = enumValue;
                    }

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::EndMenu();
        }
        ImGui::EndPopup();
        return true;
    }

    if (m_pendingClipEdit.has_value()) {
        if (m_pendingClipEdit.value().HasChanged()) {
            auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
            auto action = std::make_unique<ModifyClipAction>(groupEntity, *m_pendingClipEdit.value().reference, m_pendingClipEdit.value().mutableValue);
            m_editorLayer.PerformEditAction(std::move(action));
        }
        m_pendingClipEdit.reset();
    }

    return false;
}

void EditorPanelAnimation::DrawClipRow() {
    RowDimensions const rowDimensions = AddRow("Clips", RowOptions().ColorOverride(0xFF3D3837));

    ImGuiIO const& io = ImGui::GetIO();

    // clip track popup definition
    bool const popupShown = DrawClipPopup();

    // cancel pending selection clear if we click in a popup
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_pendingClearSelection = false;
    }

    // draw clips
    auto& animationClips = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity())->m_clips;
    for (auto&& clip : animationClips) {
        float const px = rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset;
        float const py = rowDimensions.trackBounds.Min.y;
        ImVec2 const slotP1{ px + clip->m_startFrame * m_framePixelWidth, py + 2.0f };
        ImVec2 const slotP2{ px + clip->m_endFrame * m_framePixelWidth + m_framePixelWidth, py + m_rowHeight - 2.0f };
        bool const selected = IsClipSelected(clip.get());
        unsigned int const slotColor = selected ? 0xFF00CCAA : 0xFF13BDF3;

        ImRect drawnClipRect;

        if (slotP1.x <= (m_panelBounds.GetWidth() + m_panelBounds.Min.x) && slotP2.x >= (m_panelBounds.Min.x + m_legendWidth)) {
            if (m_mouseDragging && selected) {
                // draw the moving clip
                auto context = GetSelectedClipContext(clip.get());
                ImVec2 const newClipP1{ px + context->mutableValue.m_startFrame * m_framePixelWidth + 2.0f, py + 2.0f };
                ImVec2 const newClipP2{ px + context->mutableValue.m_endFrame * m_framePixelWidth + m_framePixelWidth - 1, py + m_rowHeight - 2.0f };
                m_drawList->AddRectFilled(newClipP1, newClipP2, slotColor, 2);
                drawnClipRect.Min = newClipP1;
                drawnClipRect.Max = newClipP2;
            }
            if (io.KeyAlt || !(m_mouseDragging && selected)) {
                // draw the current keyframe
                m_drawList->AddRectFilled(slotP1, slotP2, slotColor, 2);
                drawnClipRect.Min = slotP1;
                drawnClipRect.Max = slotP2;
            }
        }

        ImRect const rects[3] = {
            ImRect{ slotP1, ImVec2{ slotP1.x + m_framePixelWidth / 2, slotP2.y } },
            ImRect{ ImVec2{ slotP2.x - m_framePixelWidth / 2, slotP1.y }, slotP2 },
            ImRect{ slotP1, slotP2 }
        };

        unsigned int const quadColor[] = { 0xFFFFFFFF, 0xFFFFFFFF, slotColor };
        if (!m_mouseDragging && m_panelBounds.Contains(io.MousePos)) {
            for (int j = 2; j >= 0; j--) {
                ImRect const& rc = rects[j];
                if (!rc.Contains(io.MousePos))
                    continue;
                m_drawList->AddRectFilled(rc.Min, rc.Max, quadColor[j], 2);
            }

            for (int j = 0; j < 3; j++) {
                ImRect const& rc = rects[j];
                if (!rc.Contains(io.MousePos))
                    continue;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    if (!IsClipSelected(clip.get())) {
                        if ((io.KeyMods & ImGuiKeyModFlags_Ctrl) == 0) {
                            ClearSelections();
                        }
                    }

                    SelectClip(clip.get());
                    m_pendingClearSelection = false;

                    // action logic
                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        m_mouseDragging = true;
                        m_mouseDragStartX = io.MousePos.x;
                        m_clipDragSection = j + 1;
                        break;
                    }
                }
            }
        }

        ImVec2 const tsize = ImGui::CalcTextSize(clip->m_name.c_str());
        ImVec2 const tpos(drawnClipRect.Min.x + (drawnClipRect.Max.x - drawnClipRect.Min.x - tsize.x) / 2, drawnClipRect.Min.y + (drawnClipRect.Max.y - drawnClipRect.Min.y - tsize.y) / 2);
        m_drawList->AddText(tpos, 0xFFFFFFFF, clip->m_name.c_str());
    }

    // detect popup click
    if (rowDimensions.trackBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_clickedFrame = static_cast<int>((io.MousePos.x - rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset) / m_framePixelWidth);
        ImGui::OpenPopup("clip_popup");
    }
}

bool EditorPanelAnimation::DrawEventPopup() {
    if (ImGui::BeginPopup(EventPopupName)) {
        auto layout = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity());
        auto const it = ranges::find_if(layout->m_events, [&](auto const& event) {
            return event->m_frame == m_clickedFrame;
        });

        EventContext* eventContext = nullptr;
        if (std::end(layout->m_events) != it) {
            eventContext = GetSelectedEventContext(it->get());
        }

        if (!eventContext && ImGui::MenuItem("Add")) {
            auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
            std::unique_ptr<IEditorAction> action = std::make_unique<AddEventAction>(groupEntity, m_clickedFrame, "");
            action->Do();
            m_editorLayer.AddEditAction(std::move(action));
        }

        if (!m_selections.empty() && ImGui::MenuItem("Delete")) {
            DeleteSelections();
        }

        if (eventContext && ImGui::BeginMenu("Edit")) {
            static char buf[1024];
            if (!m_pendingEventEdit.has_value()) {
                m_pendingEventEdit = EditContext<AnimationEvent>(eventContext->event);
                snprintf(buf, 1024, "%s", eventContext->event->m_name.c_str());
            }

            if (ImGui::InputText("Event Name", buf, 1024)) {
                m_pendingEventEdit.value().mutableValue.m_name = std::string(buf);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
        return true;
    }

    if (m_pendingEventEdit.has_value()) {
        if (m_pendingEventEdit.value().HasChanged()) {
            auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
            auto action = std::make_unique<ModifyEventAction>(groupEntity, *m_pendingEventEdit.value().reference, m_pendingEventEdit.value().mutableValue);
            m_editorLayer.PerformEditAction(std::move(action));
        }
        m_pendingEventEdit.reset();
    }

    return false;
}

void EditorPanelAnimation::DrawEventsRow() {
    ImGuiIO const& io = ImGui::GetIO();
    RowDimensions const rowDimensions = AddRow("Events", RowOptions().ColorOverride(0xFF3D3837));

    ImU32 const eventColor = 0xFFc4c4c4;
    ImU32 const eventColorSelected = 0xFFFFa2a2;

    bool const popupShown = DrawEventPopup();

    // cancel pending selection clear if we click in a popup
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_pendingClearSelection = false;
    }

    // draw events
    auto& animationEvents = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity())->m_events;
    for (auto& event : animationEvents) {
        float const px = rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset;
        float const py = rowDimensions.trackBounds.Min.y;
        ImVec2 const keyP1{ px + event->m_frame * m_framePixelWidth + 2, py + 2 };
        ImVec2 const keyP2{ px + event->m_frame * m_framePixelWidth + m_framePixelWidth - 1, py + m_rowHeight - 2 };
        ImRect const keyRect{ keyP1, keyP2 };
        bool const selected = IsEventSelected(event.get());
        if (keyP1.x <= (m_panelBounds.GetWidth() + m_panelBounds.Min.x) && keyP2.x >= (m_panelBounds.Min.x + m_legendWidth)) {
            if (m_mouseDragging && selected) {
                // draw the moving keyframe
                auto context = GetSelectedEventContext(event.get());
                ImVec2 const newEventP1{ px + context->mutableValue.m_frame * m_framePixelWidth + 2, py + 2 };
                ImVec2 const newEventP2{ px + context->mutableValue.m_frame * m_framePixelWidth + m_framePixelWidth - 1, py + m_rowHeight - 2 };
                m_drawList->AddRectFilled(newEventP1, newEventP2, selected ? eventColorSelected : eventColor, 4);
            }
            if (io.KeyAlt || !(m_mouseDragging && selected)) {
                // draw the current keyframe
                m_drawList->AddRectFilled(keyP1, keyP2, selected ? eventColorSelected : eventColor, 4);
            }
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (keyRect.Contains(io.MousePos)) {
                if ((io.KeyMods & ImGuiKeyModFlags_Ctrl) == 0) {
                    ClearSelections();
                }

                // clicked on a keyframe with left or right mouse buttons
                SelectEvent(event.get());
                m_pendingClearSelection = false;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // left mouse clicks can grab selected keyframes
                    m_mouseDragging = true;
                    m_mouseDragStartX = io.MousePos.x;
                }
            }
        }
    }

    // if we right clicked on this track we pop up a context dialog
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (rowDimensions.trackBounds.Contains(io.MousePos)) {
            m_clickedFrame = static_cast<int>((io.MousePos.x - rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset) / m_framePixelWidth);
            ImGui::OpenPopup(EventPopupName);
        }
    }
}

bool EditorPanelAnimation::DrawKeyframePopup() {
    if (ImGui::BeginPopup(KeyframePopupName)) {
        auto const child = m_group->GetChildren()[m_clickedChildIdx];
        auto const childEntity = child->GetLayoutEntity();
        auto& childTracks = childEntity->m_tracks;
        AnimationTrack* trackPtr = nullptr;
        bool keyframeAtFrame = false;

        if (m_clickedChildTarget != AnimationTrack::Target::Unknown) {
            trackPtr = childTracks.at(m_clickedChildTarget).get();
            keyframeAtFrame = trackPtr->GetKeyframe(m_clickedFrame) != nullptr;
        } else {
            for (auto& [target, track] : childTracks) {
                if (track->GetKeyframe(m_clickedFrame)) {
                    keyframeAtFrame = true;
                    break;
                }
            }
        }

        if (!keyframeAtFrame && ImGui::MenuItem("Add")) {
            if (m_clickedChildTarget != AnimationTrack::Target::Unknown) {
                // we clicked on a specific track
                auto const currentValue = trackPtr->GetValueAtFrame(static_cast<float>(m_clickedFrame));
                std::unique_ptr<IEditorAction> action = std::make_unique<AddKeyframeAction>(childEntity, m_clickedChildTarget, m_clickedFrame, currentValue, moth_ui::InterpType::Linear);
                action->Do();
                m_editorLayer.AddEditAction(std::move(action));
            } else {
                // clicked on the main track. create keyframes on all float tracks
                std::unique_ptr<CompositeAction> compositeAction = std::make_unique<CompositeAction>();
                for (auto&& target : AnimationTrack::ContinuousTargets) {
                    trackPtr = childTracks.at(target).get();
                    if (nullptr == trackPtr->GetKeyframe(m_clickedFrame)) {
                        // only add a new frame if one doesnt exist
                        auto const currentValue = trackPtr->GetValueAtFrame(static_cast<float>(m_clickedFrame));
                        auto action = std::make_unique<AddKeyframeAction>(childEntity, target, m_clickedFrame, currentValue, moth_ui::InterpType::Linear);
                        action->Do();
                        compositeAction->GetActions().push_back(std::move(action));
                    }
                }
                m_editorLayer.AddEditAction(std::move(compositeAction));
            }
        }

        if (!m_selections.empty() && ImGui::MenuItem("Delete")) {
            DeleteSelections();
        }

        if (!m_selections.empty() && ImGui::BeginMenu("Interp")) {
            bool multipleInterps = false;
            moth_ui::InterpType selectedInterp = moth_ui::InterpType::Unknown;
            for (auto&& context : m_selections) {
                if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
                    if (keyframeContext->current->m_interpType != selectedInterp) {
                        if (selectedInterp == moth_ui::InterpType::Unknown) {
                            selectedInterp = keyframeContext->current->m_interpType;
                        } else {
                            multipleInterps = true;
                        }
                    }
                }
            }
            if (multipleInterps) {
                ImGui::RadioButton("(multiple values)", multipleInterps);
            }
            for (size_t i = 0; i < magic_enum::enum_count<moth_ui::InterpType>(); ++i) {
                auto const interpType = magic_enum::enum_value<moth_ui::InterpType>(i);
                if (interpType == moth_ui::InterpType::Unknown) {
                    continue;
                }
                std::string const interpName(magic_enum::enum_name(interpType));
                bool checked = selectedInterp == interpType;
                if (ImGui::RadioButton(interpName.c_str(), checked)) {
                    auto compositeAction = std::make_unique<CompositeAction>();
                    for (auto&& context : m_selections) {
                        if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
                            auto action = MakeChangeValueAction(keyframeContext->current->m_interpType, keyframeContext->current->m_interpType, interpType, nullptr);
                            compositeAction->GetActions().push_back(std::move(action));
                        }
                    }
                    if (!compositeAction->GetActions().empty()) {
                        m_editorLayer.PerformEditAction(std::move(compositeAction));
                    }
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
        return true;
    }

    return false;
}

void EditorPanelAnimation::DrawChildTrack(int childIndex, std::shared_ptr<Node> child) {
    ImGuiIO const& io = ImGui::GetIO();

    auto const childEntity = child->GetLayoutEntity();
    auto& childTracks = childEntity->m_tracks;
    bool const selected = m_editorLayer.IsSelected(child);
    bool expanded = IsExpanded(child);

    RowOptions rowOptions;
    rowOptions.expandable = true;
    rowOptions.expanded = expanded;
    if (selected) {
        rowOptions.colorOverride = 0x55FF3333;
    }
    RowDimensions const rowDimensions = AddRow(GetChildLabel(childIndex), rowOptions);

    if (rowDimensions.buttonBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        expanded = !expanded;
    }

    SetExpanded(child, expanded);

    if (rowDimensions.labelBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!io.KeyCtrl) {
            m_editorLayer.ClearSelection();
        }
        if (m_editorLayer.IsSelected(child)) {
            m_editorLayer.RemoveSelection(child);
        } else {
            m_editorLayer.AddSelection(child);
        }
    }

    ImU32 const keyframeColor = 0xFFc4c4c4;
    ImU32 const keyframeColorSelected = 0xFFFFa2a2;

    for (auto& [target, track] : childTracks) {
        ImRect subTrackBounds = rowDimensions.trackBounds;

        if (expanded) {
            RowDimensions const subRowDimensions = AddRow(GetTrackLabel(target), RowOptions().Indented(true));
            subTrackBounds = subRowDimensions.trackBounds;
        }

        ImRect const childTrackBounds{ subTrackBounds.Min + ImVec2{ rowDimensions.trackOffset, 0 }, subTrackBounds.Max + ImVec2{ rowDimensions.trackOffset, 0 } };

        for (auto& keyframe : track->m_keyframes) {
            ImVec2 const keyP1{ childTrackBounds.Min.x + keyframe->m_frame * m_framePixelWidth + 2, childTrackBounds.Min.y + 2 };
            ImVec2 const keyP2{ childTrackBounds.Min.x + keyframe->m_frame * m_framePixelWidth + m_framePixelWidth - 1, childTrackBounds.Max.y - 2 };
            ImRect const keyRect{ keyP1, keyP2 };
            bool const selected = IsKeyframeSelected(childEntity, target, keyframe->m_frame);
            if (keyP1.x <= (m_panelBounds.GetWidth() + m_panelBounds.Min.x) && keyP2.x >= (m_panelBounds.Min.x + m_legendWidth)) {
                if (m_mouseDragging && selected) {
                    // draw the moving keyframe
                    auto context = GetSelectedKeyframeContext(childEntity, target, keyframe->m_frame);
                    ImVec2 const oldKeyP1{ childTrackBounds.Min.x + context->mutableFrame * m_framePixelWidth + 2, childTrackBounds.Min.y + 2 };
                    ImVec2 const oldKeyP2{ childTrackBounds.Min.x + context->mutableFrame * m_framePixelWidth + m_framePixelWidth - 1, childTrackBounds.Max.y - 2 };
                    m_drawList->AddRectFilled(oldKeyP1, oldKeyP2, keyframeColorSelected, 4);
                }
                if (io.KeyAlt || !(m_mouseDragging && selected)) {
                    // draw the current keyframe
                    m_drawList->AddRectFilled(keyP1, keyP2, selected ? keyframeColorSelected : keyframeColor, 4);
                }
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (keyRect.Contains(io.MousePos)) {
                    if (!IsKeyframeSelected(childEntity, target, keyframe->m_frame)) {
                        if ((io.KeyMods & ImGuiKeyModFlags_Ctrl) == 0) {
                            if (expanded) {
                                ClearSelections();
                            } else {
                                FilterKeyframeSelections(childEntity, keyframe->m_frame);
                            }
                        }
                    }

                    // clicked on a keyframe with left or right mouse buttons
                    SelectKeyframe(childEntity, target, keyframe->m_frame);
                    m_pendingClearSelection = false;

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        // left mouse clicks can grab selected keyframes
                        m_mouseDragging = true;
                        m_mouseDragStartX = io.MousePos.x;
                    }
                }

                // if we right clicked on this track we pop up a context dialog
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    if (subTrackBounds.Contains(io.MousePos)) {
                        m_clickedChildIdx = childIndex;
                        m_clickedChildTarget = expanded ? target : AnimationTrack::Target::Unknown;
                        m_clickedFrame = static_cast<int>((io.MousePos.x - subTrackBounds.Min.x + rowDimensions.trackOffset) / m_framePixelWidth);
                        ImGui::OpenPopup(KeyframePopupName);
                    }
                }
            }

            //if (DoBoxSelect) {
            //    if (keyRect.Overlaps(boxSelectBox)) {
            //        SelectKeyframe(childEntity, target, keyframe.m_frame);
            //    }
            //}
        }
    }
}

void EditorPanelAnimation::DrawTrackRows() {
    bool const popupShown = DrawKeyframePopup();

    // cancel pending selection clear if we click in a popup
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_pendingClearSelection = false;
    }

    int childIndex = 0;
    for (auto& child : m_group->GetChildren()) {
        DrawChildTrack(childIndex, child);
        ++childIndex;
    }
}

void EditorPanelAnimation::DrawHorizScrollBar() {
    ImGuiIO const& io = ImGui::GetIO();
    float const canvasWidth = m_panelBounds.GetWidth();
    ImVec2 const scrollBarSize{ canvasWidth, 14.f };
    ImGui::InvisibleButton("scrollBar", scrollBarSize);
    ImVec2 const scrollBarMin = ImGui::GetItemRectMin();
    ImVec2 const scrollBarMax = ImGui::GetItemRectMax();

    float const frameCount = static_cast<float>(ImMax(m_maxFrame - m_minFrame, 1));
    float const visibleFrameCount = floorf((canvasWidth - m_legendWidth) / m_framePixelWidth);
    float const barWidthRatio = ImMin(visibleFrameCount / frameCount, 1.0f);
    float const barWidthInPixels = barWidthRatio * (canvasWidth - m_legendWidth);

    float const startFrameOffset = ((static_cast<int>(m_firstVisibleFrame) - m_minFrame) / frameCount) * (canvasWidth - m_legendWidth);
    ImVec2 const scrollBarA{ scrollBarMin.x + m_legendWidth, scrollBarMin.y - 2 };
    ImVec2 const scrollBarB{ scrollBarMin.x + canvasWidth, scrollBarMax.y - 1 };
    ImRect const scrollBarRect{ scrollBarA, scrollBarB };
    bool const inScrollBar = scrollBarRect.Contains(io.MousePos);

    // disabled these for taste
    //m_drawList->AddRectFilled(scrollBarA, scrollBarB, 0xFF222222, 0); // light square background
    //m_drawList->AddRectFilled(scrollBarA, scrollBarB, 0xFF101010, 8); // rounded darker background

    // middle section of scroll bar
    ImVec2 const scrollBarC{ scrollBarMin.x + m_legendWidth + startFrameOffset, scrollBarMin.y };
    ImVec2 const scrollBarD{ scrollBarMin.x + m_legendWidth + barWidthInPixels + startFrameOffset, scrollBarMax.y - 2 };
    m_drawList->AddRectFilled(scrollBarC, scrollBarD, (inScrollBar || m_movingScrollBar) ? 0xFF606060 : 0xFF505050, 6);

    // left and right handles
    ImRect const barHandleLeft{ scrollBarC, ImVec2{ scrollBarC.x + 14, scrollBarD.y } };
    ImRect const barHandleRight(ImVec2{ scrollBarD.x - 14, scrollBarC.y }, scrollBarD);
    bool const onLeft = barHandleLeft.Contains(io.MousePos);
    bool const onRight = barHandleRight.Contains(io.MousePos);
    m_drawList->AddRectFilled(barHandleLeft.Min, barHandleLeft.Max, (onLeft || m_hScrollGrabbedLeft) ? 0xFFAAAAAA : 0xFF666666, 6);
    m_drawList->AddRectFilled(barHandleRight.Min, barHandleRight.Max, (onRight || m_hScrollGrabbedRight) ? 0xFFAAAAAA : 0xFF666666, 6);

    if (m_hScrollGrabbedRight) {
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedRight = false;
        } else {
            float const barNewWidth = ImMax(barWidthInPixels + io.MouseDelta.x, m_hScrollMinSize);
            float const barRatio = barNewWidth / barWidthInPixels;
            m_framePixelWidth = m_framePixelWidth / barRatio;
            float const newVisibleFrameCount = visibleFrameCount / barRatio;
            int const lastFrame = static_cast<int>(m_firstVisibleFrame + newVisibleFrameCount);
            if (lastFrame > m_maxFrame) {
                m_framePixelWidth = (canvasWidth - m_legendWidth) / static_cast<float>(m_maxFrame - m_firstVisibleFrame);
            }
        }
    } else if (m_hScrollGrabbedLeft) {
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedLeft = false;
        } else {
            if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
                float const barNewWidth = ImMax(barWidthInPixels - io.MouseDelta.x, m_hScrollMinSize);
                float const barRatio = barNewWidth / barWidthInPixels;
                m_framePixelWidth = m_framePixelWidth / barRatio;
                float const newVisibleFrameCount = visibleFrameCount / barRatio;
                float newFirstFrame = m_firstVisibleFrame + newVisibleFrameCount - visibleFrameCount;
                newFirstFrame = ImClamp(newFirstFrame, static_cast<float>(m_minFrame), ImMax(static_cast<float>(m_maxFrame) - newVisibleFrameCount, static_cast<float>(m_minFrame)));
                if (newFirstFrame != m_firstVisibleFrame) {
                    m_firstVisibleFrame = newFirstFrame;
                }
            }
        }
    } else {
        if (m_movingScrollBar) {
            if (!io.MouseDown[ImGuiMouseButton_Left]) {
                m_movingScrollBar = false;
            } else {
                float const framesPerPixelInBar = barWidthInPixels / static_cast<float>(visibleFrameCount);
                m_firstVisibleFrame = static_cast<int>((io.MousePos.x - m_panningViewSource.x) / framesPerPixelInBar) - m_panningViewFrame;
                m_firstVisibleFrame = ImClamp(m_firstVisibleFrame, static_cast<float>(m_minFrame), ImMax(static_cast<float>(m_maxFrame) - visibleFrameCount, static_cast<float>(m_minFrame)));
            }
        } else {
            ImRect const scrollBarThumb{ scrollBarC, scrollBarD };
            if (scrollBarThumb.Contains(io.MousePos) && ImGui::IsMouseClicked(0) && !m_movingCurrentFrame && m_movingEntry == -1) {
                m_movingScrollBar = true;
                m_panningViewSource = io.MousePos;
                m_panningViewFrame = -m_firstVisibleFrame;
            }
            if (!m_hScrollGrabbedRight && onRight && ImGui::IsMouseClicked(0))
                m_hScrollGrabbedRight = true;
            if (!m_hScrollGrabbedLeft && onLeft && ImGui::IsMouseClicked(0))
                m_hScrollGrabbedLeft = true;
        }
    }
}

int EditorPanelAnimation::CalcNumRows() const {
    int count = 3; // frames + clips + events;
    count += m_group->GetChildCount();
    for (auto& [p, extra] : m_extraData) {
        if (auto child = extra.ptr.lock()) {
            if (extra.expanded) {
                count += static_cast<int>(child->GetLayoutEntity()->m_tracks.size());
            }
        }
    }
    return count;
}

bool EditorPanelAnimation::IsAnyPopupOpen() const {
    return ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
}

void EditorPanelAnimation::DrawCursor() {
    if (m_currentFrame >= m_firstVisibleFrame && m_currentFrame <= m_maxFrame) {
        float const cursorLeft = m_panelBounds.Min.x + m_legendWidth + (m_currentFrame - m_minFrame) * m_framePixelWidth;
        float const cursorOffset = cursorLeft + m_framePixelWidth / 2;
        m_drawList->AddLine(ImVec2(cursorOffset, m_panelBounds.Min.y), ImVec2(cursorOffset, m_panelBounds.Max.y), 0xA02A2AFF, m_framePixelWidth - 1);
        char tmps[512];
        ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", m_currentFrame);
        m_drawList->AddText(ImVec2(cursorLeft + 3.0f, m_panelBounds.Min.y), 0xFFFFFFFF, tmps);
    }
}

void EditorPanelAnimation::DrawWidget() {
    m_group = m_editorLayer.GetRoot();
    SanitizeExtraData();
    m_rowCounter = 0;

    ImGui::PushItemWidth(130);
    ImGui::InputInt("Frame Min", &m_minFrame);
    ImGui::SameLine();
    ImGui::InputInt("Frame ", &m_currentFrame);
    ImGui::SameLine();
    ImGui::InputInt("Frame Max", &m_maxFrame);
    ImGui::PopItemWidth();

    m_minFrame = std::max(0, m_minFrame);
    m_currentFrame = std::clamp(m_currentFrame, m_minFrame, m_maxFrame);

    int const targetRowCount = CalcNumRows();
    m_windowBounds.Min = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
    m_windowBounds.Max = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax();
    ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
    float const panelHeight = m_rowHeight * targetRowCount;
    float const vScrollBarWidth = panelHeight > canvasSize.y ? 18.0f : 0.0f;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::BeginChildFrame(889, ImVec2{ canvasSize.x, canvasSize.y - 18.0f });
    ImGui::InvisibleButton("contentBar", ImVec2(canvasSize.x - vScrollBarWidth, static_cast<float>(panelHeight)));
    m_panelBounds.Min = ImGui::GetItemRectMin();
    m_panelBounds.Max = ImGui::GetItemRectMax();
    m_drawList = ImGui::GetWindowDrawList();

    ImRect trackBounds;
    trackBounds.Min = { m_panelBounds.Min.x + m_legendWidth, m_panelBounds.Min.y + m_rowHeight };
    trackBounds.Max = m_panelBounds.Max;
    m_pendingClearSelection = trackBounds.Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    DrawFrameRow();
    DrawClipRow();
    DrawEventsRow();
    DrawTrackRows();
    DrawCursor();

    ImGui::EndChildFrame();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    DrawHorizScrollBar();

    m_drawList = nullptr;

    if (m_pendingClearSelection) {
        ClearSelections();
    }

    UpdateMouseDragging();
}

void EditorPanelAnimation::UpdateMouseDragging() {
    if (m_mouseDragging) {
        float const dragDelta = ImGui::GetMousePos().x - m_mouseDragStartX;
        int const frameDelta = static_cast<int>(dragDelta / m_framePixelWidth);

        if (frameDelta != 0) {
            for (auto& context : m_selections) {
                if (auto clipContext = std::get_if<ClipContext>(&context)) {
                    // if we're dragging a single clip we check the edge we're dragging, otherwise we move the whole thing
                    if (m_selections.size() == 1) {
                        int& l = clipContext->mutableValue.m_startFrame;
                        int& r = clipContext->mutableValue.m_endFrame;
                        if (m_clipDragSection & 1)
                            l = clipContext->clip->m_startFrame + frameDelta;
                        if (m_clipDragSection & 2)
                            r = clipContext->clip->m_endFrame + frameDelta;
                        if (l < 0) {
                            if (m_clipDragSection & 2)
                                r -= l;
                            l = 0;
                        }
                        if (m_clipDragSection & 1 && l > r)
                            l = r;
                        if (m_clipDragSection & 2 && r < l)
                            r = l;
                    } else {
                        clipContext->mutableValue.m_startFrame = clipContext->clip->m_startFrame + frameDelta;
                        clipContext->mutableValue.m_endFrame = clipContext->clip->m_endFrame + frameDelta;
                    }
                } else if (auto eventContext = std::get_if<EventContext>(&context)) {
                    eventContext->mutableValue.m_frame = eventContext->event->m_frame + frameDelta;
                } else if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
                    keyframeContext->mutableFrame = keyframeContext->current->m_frame + frameDelta;
                }
            }
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_mouseDragging = false;

            auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
            auto compositeAction = std::make_unique<CompositeAction>();
            for (auto& context : m_selections) {
                if (auto clipContext = std::get_if<ClipContext>(&context)) {
                    if (clipContext->clip->m_startFrame != clipContext->mutableValue.m_startFrame || clipContext->clip->m_endFrame != clipContext->mutableValue.m_endFrame) {
                        auto moveAction = std::make_unique<ModifyClipAction>(groupEntity, *clipContext->clip, clipContext->mutableValue);
                        compositeAction->GetActions().push_back(std::move(moveAction));
                    }
                } else if (auto eventContext = std::get_if<EventContext>(&context)) {
                    if (eventContext->event->m_frame != eventContext->mutableValue.m_frame) {
                        auto moveAction = std::make_unique<ModifyEventAction>(groupEntity, *eventContext->event, eventContext->mutableValue);
                        compositeAction->GetActions().push_back(std::move(moveAction));
                    }
                } else if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
                    if (keyframeContext->current->m_frame != keyframeContext->mutableFrame) {
                        auto moveAction = std::make_unique<MoveKeyframeAction>(keyframeContext->entity, keyframeContext->target, keyframeContext->current->m_frame, keyframeContext->mutableFrame);
                        compositeAction->GetActions().push_back(std::move(moveAction));
                    }
                }
            }
            if (!compositeAction->GetActions().empty()) {
                if (compositeAction->GetActions().size() == 1) {
                    m_editorLayer.PerformEditAction(std::move(compositeAction->GetActions()[0]));
                } else {
                    m_editorLayer.PerformEditAction(std::move(compositeAction));
                }
            }
        }
    }
}

bool EditorPanelAnimation::IsExpanded(std::shared_ptr<moth_ui::Node> child) const {
    auto it = m_extraData.find(child.get());
    if (std::end(m_extraData) == it) {
        return false;
    }
    return it->second.expanded;
}

void EditorPanelAnimation::SetExpanded(std::shared_ptr<moth_ui::Node> child, bool expanded) {
    auto it = m_extraData.find(child.get());
    if (std::end(m_extraData) == it) {
        m_extraData.insert(std::make_pair<void*, TrackExtraData>(child.get(), { child, expanded }));
    } else {
        it->second.expanded = expanded;
    }
}

void EditorPanelAnimation::SanitizeExtraData() {
    for (auto it = std::begin(m_extraData); it != std::end(m_extraData); /* skip */) {
        if (!it->second.ptr.lock()) {
            it = m_extraData.erase(it);
        } else {
            ++it;
        }
    }
}
