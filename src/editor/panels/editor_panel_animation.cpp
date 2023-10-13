#include "common.h"
#include "editor_panel_animation.h"
#include "../editor_layer.h"
#include "../utils.h"
#include "../actions/composite_action.h"
#include "../actions/add_clip_action.h"
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
#include "moth_ui/layout/layout.h"
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

    m_framePixelWidth = 10.f;

    m_minFrame = m_editorLayer.GetConfig().MinAnimationFrame;
    m_maxFrame = m_editorLayer.GetConfig().MaxAnimationFrame;
    m_totalFrames = m_editorLayer.GetConfig().TotalAnimationFrames;
    m_currentFrame = m_editorLayer.GetConfig().CurrentAnimationFrame;

    auto layout = m_editorLayer.GetCurrentLayout();
    auto& extraData = layout->GetExtraData();
    m_persistentLayoutConfig = &extraData["animation_panel"];
    if (!m_persistentLayoutConfig->is_null()) {
        (*m_persistentLayoutConfig)["m_minFrame"].get_to(m_minFrame);
        (*m_persistentLayoutConfig)["m_maxFrame"].get_to(m_maxFrame);
        (*m_persistentLayoutConfig)["m_totalFrames"].get_to(m_totalFrames);
        (*m_persistentLayoutConfig)["m_currentFrame"].get_to(m_currentFrame);
    }
    m_editorLayer.SetSelectedFrame(m_currentFrame);

    m_hScrollFactors = { m_minFrame / static_cast<float>(m_totalFrames), m_maxFrame / static_cast<float>(m_totalFrames) };
    m_trackMetadata.clear();
}

void EditorPanelAnimation::OnShutdown() {
    m_editorLayer.GetConfig().MinAnimationFrame = m_minFrame;
    m_editorLayer.GetConfig().MaxAnimationFrame = m_maxFrame;
    m_editorLayer.GetConfig().TotalAnimationFrames = m_totalFrames;
    m_editorLayer.GetConfig().CurrentAnimationFrame = m_currentFrame;
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
        auto keyframeContext = std::get_if<KeyframeContext>(&context);
        if (!keyframeContext || (keyframeContext->entity != entity || keyframeContext->current->m_frame != frameNo)) {
            it = m_selections.erase(it);
        } else {
            ++it;
        }
    }
}

EditorPanelAnimation::RowDimensions EditorPanelAnimation::AddRow(char const* label, RowOptions const& rowOptions) {
    RowDimensions resultDimensions;

    ImVec2 const rowMin = m_scrollingPanelBounds.Min + ImVec2{ 0.0f, m_rowHeight * m_rowCounter };
    ImVec2 const rowMax = rowMin + ImVec2{ m_scrollingPanelBounds.GetWidth(), m_rowHeight };
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
        ImVec2 const labelMax{ rowMin.x + m_labelColumnWidth, rowMax.y };
        ImRect const labelBounds{ labelMin, labelMax };
        ImVec2 const textPos = labelMin + ImVec2(rowOptions.indented ? 50.0f : 3.0f, 0.0f);
        m_drawList->AddText(textPos, 0xFFFFFFFF, label);
        cursorPos += labelBounds.GetSize().x;
        resultDimensions.labelBounds = labelBounds;
    }

    ImVec2 const trackMin{ rowMin.x + m_labelColumnWidth, rowMin.y };
    ImVec2 const trackMax = rowMax;
    resultDimensions.trackBounds = ImRect{ trackMin, trackMax };
    resultDimensions.trackOffset = -static_cast<int>(m_minFrame) * m_framePixelWidth;

    ++m_rowCounter;

    return resultDimensions;
}

void EditorPanelAnimation::DrawFrameNumberRibbon() {

    ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("frameRibbon_", ImVec2(canvasSize.x, m_rowHeight));
    ImVec2 aMin = ImGui::GetItemRectMin();
    ImVec2 aMax = ImGui::GetItemRectMax();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // header frame number and lines
    int const minStep = 1;
    int const stepFactor = 5;
    int const maxStepWidth = 150;
    int const majorFrameStep = [&]() {
        int step = minStep;
        for (int i = 0; (step * m_framePixelWidth) < maxStepWidth; ++i) {
            step = static_cast<int>(std::floor(stepFactor * std::pow(2, i)));
        };
        return step;
    }();
    int const minorFrameStep = majorFrameStep / 2;

    ImVec2 const trackMin{ aMin.x + m_labelColumnWidth, aMin.y };
    ImVec2 const trackMax{ aMax.x, aMax.y };
    ImRect const trackBounds{ trackMin, trackMax };
    float const trackOffset = -static_cast<int>(m_minFrame) * m_framePixelWidth;

    for (int i = m_minFrame; i <= m_maxFrame; ++i) {
        bool const majorTick = ((i % majorFrameStep) == 0) || (i == m_maxFrame || i == m_minFrame);
        bool const minorTick = (minorFrameStep > 0) && ((i % minorFrameStep) == 0);
        float const px = trackMin.x + i * m_framePixelWidth + trackOffset;
        float const tickYOffset = majorTick ? 4.0f : (minorTick ? 10.0f : 14.0f);

        // frame edge ticks
        if (px >= trackMin.x && px <= trackMax.x) {
            ImVec2 const start{ px, trackMin.y + tickYOffset };
            ImVec2 const end{ px, trackMax.y - 1 };
            drawList->AddLine(start, end, 0xFF606060, 1);

            // frame numbers
            if (majorTick) {
                static char tmps[32];
                ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", i);
                drawList->AddText(ImVec2(px + 3.f, trackMin.y), 0xFFBBBBBB, tmps);
            }
        }
    }

    // moving current frame
    ImGuiIO const& io = ImGui::GetIO();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && trackBounds.Contains(io.MousePos)) {
        m_grabbedCurrentFrame = true;
    }

    float const frameCount = static_cast<float>(ImMax(m_maxFrame - m_minFrame, 1));
    if (m_grabbedCurrentFrame) {
        if (frameCount) {
            m_currentFrame = static_cast<int>((io.MousePos.x - trackBounds.Min.x + -trackOffset) / m_framePixelWidth);
            if (m_currentFrame < m_minFrame)
                m_currentFrame = m_minFrame;
            if (m_currentFrame >= m_maxFrame)
                m_currentFrame = m_maxFrame;
        }
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_grabbedCurrentFrame = false;
        }
    }

    // calculate cursor rect (drawn later to be on top)
    float const cursorLeft = trackBounds.Min.x + (m_currentFrame - m_minFrame) * m_framePixelWidth;
    m_cursorRect.Min = { cursorLeft, trackBounds.Min.y };
    m_cursorRect.Max = { cursorLeft + m_framePixelWidth + 1.0f, trackBounds.Min.y + canvasSize.y - m_horizontalScrollbarHeight };
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
        m_clickConsumed = true;
    }

    m_drawList->PushClipRect(rowDimensions.trackBounds.Min, rowDimensions.trackBounds.Max, true);

    float const trackStartOffsetX = rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset;
    float const trackStartOffsetY = rowDimensions.trackBounds.Min.y;

    // draw clips
    auto& animationClips = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity())->m_clips;
    for (auto&& clip : animationClips) {
        bool const selected = IsClipSelected(clip.get());
        unsigned int const slotColor = selected ? 0xFF00CCAA : 0xFF13BDF3;

        auto& clipValues = (m_mouseDragging && selected) ? GetSelectedClipContext(clip.get())->mutableValue : *clip;

        float const clipStartOffset = clipValues.m_startFrame * m_framePixelWidth;
        float const clipEndOffset = (clipValues.m_endFrame + 1) * m_framePixelWidth;
        ImVec2 const clipBoundsMin{ trackStartOffsetX + clipStartOffset, trackStartOffsetY + 2.0f };
        ImVec2 const clipBoundsMax{ trackStartOffsetX + clipEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
        m_drawList->AddRectFilled(clipBoundsMin, clipBoundsMax, slotColor, 2.0f);

        if (io.KeyAlt) {
            // alt will allow you to dupe drag the clip. so draw the original clip here if we're holding alt
            float const clipStartOffset = clip->m_startFrame * m_framePixelWidth;
            float const clipEndOffset = (clip->m_endFrame + 1) * m_framePixelWidth;
            ImVec2 const clipBoundsMin{ trackStartOffsetX + clipStartOffset, trackStartOffsetY + 2.0f };
            ImVec2 const clipBoundsMax{ trackStartOffsetX + clipEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
            m_drawList->AddRectFilled(clipBoundsMin, clipBoundsMax, slotColor, 2.0f);
        }

        float const clipEdgeWidth = m_framePixelWidth / 2;
        ImRect const rects[3] = {
            ImRect{ clipBoundsMin, ImVec2{ clipBoundsMin.x + clipEdgeWidth, clipBoundsMax.y } },
            ImRect{ ImVec2{ clipBoundsMax.x - clipEdgeWidth, clipBoundsMin.y }, clipBoundsMax },
            ImRect{ clipBoundsMin, clipBoundsMax }
        };

        unsigned int const quadColor[] = { 0xFFFFFFFF, 0xFFFFFFFF, slotColor };
        if (!m_mouseDragging && m_scrollingPanelBounds.Contains(io.MousePos)) {
            for (int i = 2; i >= 0; --i) {
                ImRect const& rc = rects[i];
                if (!rc.Contains(io.MousePos))
                    continue;

                m_drawList->AddRectFilled(rc.Min, rc.Max, quadColor[i], 2.0f);
            }

            if (io.MousePos.x >= rowDimensions.trackBounds.Min.x && io.MousePos.x <= rowDimensions.trackBounds.Max.x) {
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
                        m_clickConsumed = true;

                        // action logic
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            m_mouseDragging = true;
                            m_mouseDragStartX = io.MousePos.x;
                            m_clipDragHandle = j + 1;
                            break;
                        }
                    }
                }
            }
        }

        ImVec2 const tsize = ImGui::CalcTextSize(clip->m_name.c_str());
        ImVec2 const tpos(clipBoundsMin.x + (clipBoundsMax.x - clipBoundsMin.x - tsize.x) / 2, clipBoundsMin.y + (clipBoundsMax.y - clipBoundsMin.y - tsize.y) / 2);
        m_drawList->AddText(tpos, 0xFFFFFFFF, clip->m_name.c_str());
    }

    m_drawList->PopClipRect();

    // detect popup click
    if (rowDimensions.trackBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_clickedFrame = static_cast<int>((io.MousePos.x - rowDimensions.trackBounds.Min.x + -rowDimensions.trackOffset) / m_framePixelWidth);
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
        m_clickConsumed = true;
    }

    m_drawList->PushClipRect(rowDimensions.trackBounds.Min, rowDimensions.trackBounds.Max, true);

    float const trackStartOffsetX = rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset;
    float const trackStartOffsetY = rowDimensions.trackBounds.Min.y;

    // draw events
    auto& animationEvents = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity())->m_events;
    for (auto& event : animationEvents) {
        bool const selected = IsEventSelected(event.get());
        unsigned int const slotColor = selected ? eventColorSelected : eventColor;

        auto& eventValues = (m_mouseDragging && selected) ? GetSelectedEventContext(event.get())->mutableValue : *event;

        float const eventStartOffset = eventValues.m_frame * m_framePixelWidth;
        float const eventEndOffset = (eventValues.m_frame + 1) * m_framePixelWidth;
        ImVec2 const eventBoundsMin{ trackStartOffsetX + eventStartOffset, trackStartOffsetY + 2.0f };
        ImVec2 const eventBoundsMax{ trackStartOffsetX + eventEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
        ImRect const eventBounds{ eventBoundsMin, eventBoundsMax };
        m_drawList->AddRectFilled(eventBoundsMin, eventBoundsMax, slotColor, 2.0f);

        // tooltip to display the event name
        if (eventBounds.Contains(io.MousePos)) {
            ImGui::BeginTooltip();
            ImGui::Text("Event \"%s\"", eventValues.m_name.c_str());
            ImGui::EndTooltip();
        }

        if (io.KeyAlt) {
            // alt will allow you to dupe drag the clip. so draw the original clip here if we're holding alt
            float const eventStartOffset = event->m_frame * m_framePixelWidth;
            float const eventEndOffset = (event->m_frame + 1) * m_framePixelWidth;
            ImVec2 const eventBoundsMin{ trackStartOffsetX + eventStartOffset, trackStartOffsetY + 2.0f };
            ImVec2 const eventBoundsMax{ trackStartOffsetX + eventEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
            m_drawList->AddRectFilled(eventBoundsMin, eventBoundsMax, slotColor, 2.0f);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (eventBounds.Contains(io.MousePos)) {
                if ((io.KeyMods & ImGuiKeyModFlags_Ctrl) == 0) {
                    ClearSelections();
                }

                // clicked on a keyframe with left or right mouse buttons
                SelectEvent(event.get());
                m_clickConsumed = true;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    // left mouse clicks can grab selected keyframes
                    m_mouseDragging = true;
                    m_mouseDragStartX = io.MousePos.x;
                }
            }
        }
    }

    m_drawList->PopClipRect();

    // if we right clicked on this track we pop up a context dialog
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (rowDimensions.trackBounds.Contains(io.MousePos)) {
            m_clickedFrame = static_cast<int>((io.MousePos.x - rowDimensions.trackBounds.Min.x + -rowDimensions.trackOffset) / m_framePixelWidth);
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

    // expanding and unexpanding
    if (rowDimensions.buttonBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        expanded = !expanded;
    }

    SetExpanded(child, expanded);

    // selecting the entity for the track
    if (rowDimensions.labelBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!io.KeyCtrl) { // holding ctrl will let you add to the selection. otherwise we clear it before making the selection
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

        m_drawList->PushClipRect(subTrackBounds.Min, subTrackBounds.Max, true);

        float const trackStartOffsetX = subTrackBounds.Min.x + rowDimensions.trackOffset;
        float const trackStartOffsetY = subTrackBounds.Min.y;

        for (auto& keyframe : track->m_keyframes) {
            bool selected = IsKeyframeSelected(childEntity, target, keyframe->m_frame);

            int const frameNumber = (m_mouseDragging && selected) ? GetSelectedKeyframeContext(childEntity, target, keyframe->m_frame)->mutableFrame : keyframe->m_frame;

            float const frameStartOffset = frameNumber * m_framePixelWidth + 1.0f;
            float const frameEndOffset = (frameNumber + 1) * m_framePixelWidth;
            ImVec2 const frameBoundsMin{ trackStartOffsetX + frameStartOffset, trackStartOffsetY + 2.0f };
            ImVec2 const frameBoundsMax{ trackStartOffsetX + frameEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
            ImRect const frameBounds{ frameBoundsMin, frameBoundsMax };
            
            if (m_boxSelecting) {
                if (frameBounds.Overlaps(m_selectBox)) {
                    m_pendingBoxSelections.push_back(KeyframeContext{ childEntity, target, keyframe->m_frame });
                    selected = true;
                }
            }

            unsigned int const slotColor = selected ? keyframeColorSelected : keyframeColor;
            m_drawList->AddRectFilled(frameBoundsMin, frameBoundsMax, slotColor, 0.0f);

            if (io.KeyAlt) {
                // alt will allow you to dupe drag the clip. so draw the original clip here if we're holding alt
                float const frameStartOffset = keyframe->m_frame * m_framePixelWidth + 1.0f;
                float const frameEndOffset = (keyframe->m_frame + 1) * m_framePixelWidth;
                ImVec2 const frameBoundsMin{ trackStartOffsetX + frameStartOffset, trackStartOffsetY + 2.0f };
                ImVec2 const frameBoundsMax{ trackStartOffsetX + frameEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
                m_drawList->AddRectFilled(frameBoundsMin, frameBoundsMax, slotColor, 0.0f);
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (frameBounds.Contains(io.MousePos)) {
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
                    m_clickConsumed = true;

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
        }

        m_drawList->PopClipRect();
    }
}

void EditorPanelAnimation::DrawTrackRows() {
    bool const popupShown = DrawKeyframePopup();

    // cancel pending selection clear if we click in a popup
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_clickConsumed = true;
    }

    int childIndex = 0;
    for (auto& child : m_group->GetChildren()) {
        DrawChildTrack(childIndex, child);
        ++childIndex;
    }
}

void EditorPanelAnimation::DrawHorizScrollBar() {
    ImGuiIO const& io = ImGui::GetIO();
    float const scrollingPanelWidth = m_scrollingPanelBounds.GetWidth();
    ImGui::InvisibleButton("scrollBar", { scrollingPanelWidth, m_horizontalScrollbarHeight - 4.0f });
    ImVec2 const elementBoundsMin = ImGui::GetItemRectMin();
    ImVec2 const elementBoundsMax = ImGui::GetItemRectMax();
    ImRect const elementBounds{ elementBoundsMin, elementBoundsMax };

    // scroll track
    ImVec2 const scrollTrackMin{ elementBounds.Min.x + m_labelColumnWidth, elementBounds.Min.y - 2 };
    ImVec2 const scrollTrackMax{ elementBounds.Min.x + scrollingPanelWidth, elementBounds.Max.y - 1 };
    ImRect const scrollTrackBounds{ scrollTrackMin, scrollTrackMax };

    // scroll bar
    ImVec2 const scrollBarMin = { scrollTrackBounds.Min.x + scrollTrackBounds.GetWidth() * m_hScrollFactors.x, elementBounds.Min.y };
    ImVec2 const scrollBarMax = { scrollTrackBounds.Min.x + scrollTrackBounds.GetWidth() * m_hScrollFactors.y, elementBounds.Max.y - 2 };
    ImRect const scrollBarBounds{ scrollBarMin, scrollBarMax };

    // handles
    float const handleWidth = 14.0f;
    ImRect const barHandleLeft{ scrollBarMin, { scrollBarMin.x + handleWidth, scrollBarMax.y } };
    ImRect const barHandleRight{ { scrollBarMax.x - handleWidth, scrollBarMin.y }, scrollBarMax };

    bool const mouseInScrollBar = scrollTrackBounds.Contains(io.MousePos);
    bool const mouseInLeftHandle = barHandleLeft.Contains(io.MousePos);
    bool const mouseInRightHandle = barHandleRight.Contains(io.MousePos);

    // draw the scroll bar and its handles
    m_drawList->AddRectFilled(scrollBarMin, scrollBarMax, (mouseInScrollBar || m_hScrollGrabbedBar) ? 0xFF606060 : 0xFF505050, 6);
    m_drawList->AddRectFilled(barHandleLeft.Min, barHandleLeft.Max, (mouseInLeftHandle || m_hScrollGrabbedLeft) ? 0xFFAAAAAA : 0xFF666666, 6);
    m_drawList->AddRectFilled(barHandleRight.Min, barHandleRight.Max, (mouseInRightHandle || m_hScrollGrabbedRight) ? 0xFFAAAAAA : 0xFF666666, 6);

    if (m_hScrollGrabbedRight) {
        // moving the max scroll handle
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedRight = false;
        } else {
            if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
                float const deltaToScrollFactor = io.MouseDelta.x / scrollTrackBounds.GetWidth();
                m_hScrollFactors.y = std::clamp(m_hScrollFactors.y + deltaToScrollFactor, 0.0f, 1.0f);
                m_maxFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.y);
            }
        }
    } else if (m_hScrollGrabbedLeft) {
        // moving the min scroll handle
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedLeft = false;
        } else {
            if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
                float const deltaToScrollFactor = io.MouseDelta.x / scrollTrackBounds.GetWidth();
                m_hScrollFactors.x = std::clamp(m_hScrollFactors.x + deltaToScrollFactor, 0.0f, 1.0f);
                m_minFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.x);
            }
        }
    } else {
        if (m_hScrollGrabbedBar) {
            // moving the scroll bar as a whole
            if (!io.MouseDown[ImGuiMouseButton_Left]) {
                m_hScrollGrabbedBar = false;
            } else {
                if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
                    float const deltaToScrollFactor = io.MouseDelta.x / scrollTrackBounds.GetWidth();
                    ImVec2 const newScrollFactors = m_hScrollFactors + ImVec2{ deltaToScrollFactor, deltaToScrollFactor };
                    if (newScrollFactors.x < 0.0f) {
                        float const clampedMove = m_hScrollFactors.x;
                        m_hScrollFactors.x = 0.0f;
                        m_hScrollFactors.y -= clampedMove;
                    } else if (newScrollFactors.y > 1.0f) {
                        float const clampedMove = m_hScrollFactors.y - 1.0f;
                        m_hScrollFactors.x += clampedMove;
                        m_hScrollFactors.y = 1.0f;
                    } else {
                        m_hScrollFactors.x = std::clamp(m_hScrollFactors.x + deltaToScrollFactor, 0.0f, 1.0f);
                        m_hScrollFactors.y = std::clamp(m_hScrollFactors.y + deltaToScrollFactor, 0.0f, 1.0f);
                    }

                    m_minFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.x);
                    m_maxFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.y);
                }
            }
        } else {
            if (scrollBarBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_grabbedCurrentFrame) {
                m_hScrollGrabbedBar = true;
            }
            if (!m_hScrollGrabbedRight && mouseInRightHandle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_hScrollGrabbedRight = true;
            }
            if (!m_hScrollGrabbedLeft && mouseInLeftHandle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                m_hScrollGrabbedLeft = true;
            }
        }
    }

    // calculate the scale of a single frame
    m_framePixelWidth = scrollTrackBounds.GetWidth() / (m_maxFrame - m_minFrame + 1); // +1 so that the max frame is "in" the track
}

int EditorPanelAnimation::CalcNumRows() const {
    int count = 3; // frames + clips + events;
    count += m_group->GetChildCount();
    for (auto& [p, metadata] : m_trackMetadata) {
        if (auto child = metadata.ptr.lock()) {
            if (metadata.expanded) {
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
    if (m_currentFrame >= m_minFrame && m_currentFrame <= m_maxFrame) {
        m_drawList->AddRectFilled(m_cursorRect.Min, m_cursorRect.Max, 0xA02A2AFF, 0.0f);
        char tmps[512];
        ImFormatString(tmps, IM_ARRAYSIZE(tmps), "%d", m_currentFrame);
        m_drawList->AddText(ImVec2(m_cursorRect.Min.x + 3.0f, m_cursorRect.Min.y), 0xFFFFFFFF, tmps);
    }
}

void EditorPanelAnimation::DrawFrameRangeSettings() {
    ImGui::PushItemWidth(130);
    ImGui::InputInt("Current Frame ", &m_currentFrame);
    ImGui::SameLine();
    if (ImGui::InputInt("Min Frame", &m_minFrame)) {
        m_hScrollFactors.x = m_minFrame / static_cast<float>(m_totalFrames);
    }
    ImGui::SameLine();

    if (ImGui::InputInt("Max Frame", &m_maxFrame)) {
        m_hScrollFactors.y = m_maxFrame / static_cast<float>(m_totalFrames);
    }
    ImGui::SameLine();
    if (ImGui::InputInt("Total Frames", &m_totalFrames)) {
        m_hScrollFactors.x = m_minFrame / static_cast<float>(m_totalFrames);
        m_hScrollFactors.y = m_maxFrame / static_cast<float>(m_totalFrames);
    }
    ImGui::PopItemWidth();
}

void EditorPanelAnimation::DrawWidget() {
    m_group = m_editorLayer.GetRoot();
    SanitizeExtraData();

    m_rowCounter = 0;

    m_minFrame = std::max(0, m_minFrame);
    m_currentFrame = std::min(m_totalFrames, m_currentFrame);

    // keep these up to date
    if (m_persistentLayoutConfig) {
        (*m_persistentLayoutConfig)["m_minFrame"] = m_minFrame;
        (*m_persistentLayoutConfig)["m_maxFrame"] = m_maxFrame;
        (*m_persistentLayoutConfig)["m_currentFrame"] = m_currentFrame;
        (*m_persistentLayoutConfig)["m_totalFrames"] = m_totalFrames;
    }

    m_windowBounds.Min = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
    m_windowBounds.Max = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax();

    DrawFrameRangeSettings();
    DrawFrameNumberRibbon();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 0 });
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);

    // prepare the vertical scrolling panel
    ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
    int const targetRowCount = CalcNumRows();
    float const panelHeight = m_rowHeight * targetRowCount;
    float const vScrollBarWidth = panelHeight > canvasSize.y ? m_verticalScrollbarWidth : 0.0f;
    ImGui::BeginChildFrame(889, ImVec2{ canvasSize.x, canvasSize.y - m_horizontalScrollbarHeight });
    ImGui::InvisibleButton("contentBar", ImVec2(canvasSize.x - vScrollBarWidth, static_cast<float>(panelHeight)));
    m_scrollingPanelBounds.Min = ImGui::GetItemRectMin();
    m_scrollingPanelBounds.Max = ImGui::GetItemRectMax();

    // clicking on blank track area will clear the selection.
    // clickable objects will reset this value if they are interacted with.
    ImRect trackAreaBounds;
    trackAreaBounds.Min = { m_scrollingPanelBounds.Min.x + m_labelColumnWidth, m_scrollingPanelBounds.Min.y };
    trackAreaBounds.Max = m_scrollingPanelBounds.Max;
    m_clickConsumed = !(trackAreaBounds.Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(ImGuiMouseButton_Left));

    m_drawList = ImGui::GetWindowDrawList();

    m_pendingBoxSelections.clear();

    DrawClipRow();
    DrawEventsRow();
    DrawTrackRows();

    if (!m_clickConsumed) {
        m_selectBoxStart = ImGui::GetMousePos();
        m_boxSelectStarted = true;
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (m_boxSelectStarted) {
            m_selectBoxEnd = ImGui::GetMousePos();

            m_selectBox.Min.x = std::min(m_selectBoxStart.x, m_selectBoxEnd.x);
            m_selectBox.Min.y = std::min(m_selectBoxStart.y, m_selectBoxEnd.y);
            m_selectBox.Max.x = std::max(m_selectBoxStart.x, m_selectBoxEnd.x);
            m_selectBox.Max.y = std::max(m_selectBoxStart.y, m_selectBoxEnd.y);

            if (m_selectBox.GetArea() > 30) {
                m_boxSelecting = true;
            }

            if (m_boxSelecting) {
                m_drawList->AddRect(m_selectBox.Min, m_selectBox.Max, 0xFF00FFFF);
            }
        }
    } else {
        m_boxSelectStarted = false;
        m_boxSelecting = false;

        // commit selections
        for (auto& pendingSelection : m_pendingBoxSelections) {
            SelectKeyframe(pendingSelection.entity, pendingSelection.target, pendingSelection.mutableFrame);
        }
    }

    ImGui::EndChildFrame();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    DrawCursor();

    DrawHorizScrollBar();

    m_drawList = nullptr;

    // now we clear selections if nothing reset this bool
    auto& io = ImGui::GetIO();
    if (!m_clickConsumed && !io.KeyCtrl) {
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
                        if (m_clipDragHandle & 1)
                            l = clipContext->clip->m_startFrame + frameDelta;
                        if (m_clipDragHandle & 2)
                            r = clipContext->clip->m_endFrame + frameDelta;
                        if (l < 0) {
                            if (m_clipDragHandle & 2)
                                r -= l;
                            l = 0;
                        }
                        if (m_clipDragHandle & 1 && l > r)
                            l = r;
                        if (m_clipDragHandle & 2 && r < l)
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
            // commit actions
            m_mouseDragging = false;

            // move the original element to the new position
            // end if we hold alt down here we add a new element into the originals place.
            // this way we dont have to juggle with selections and the moved elements
            // remain selected.

            auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
            auto compositeAction = std::make_unique<CompositeAction>();
            for (auto& context : m_selections) {
                if (auto clipContext = std::get_if<ClipContext>(&context)) {
                    if (clipContext->clip->m_startFrame != clipContext->mutableValue.m_startFrame || clipContext->clip->m_endFrame != clipContext->mutableValue.m_endFrame) {
                        compositeAction->GetActions().push_back(std::make_unique<ModifyClipAction>(groupEntity, *clipContext->clip, clipContext->mutableValue));
                        if (ImGui::GetIO().KeyAlt) {
                            compositeAction->GetActions().push_back(std::make_unique<AddClipAction>(groupEntity, *clipContext->clip));
                        }
                    }
                } else if (auto eventContext = std::get_if<EventContext>(&context)) {
                    if (eventContext->event->m_frame != eventContext->mutableValue.m_frame) {
                        compositeAction->GetActions().push_back(std::make_unique<ModifyEventAction>(groupEntity, *eventContext->event, eventContext->mutableValue));
                        if (ImGui::GetIO().KeyAlt) {
                            compositeAction->GetActions().push_back(std::make_unique<AddEventAction>(groupEntity, eventContext->event->m_frame, eventContext->event->m_name));
                        }
                    }
                } else if (auto keyframeContext = std::get_if<KeyframeContext>(&context)) {
                    if (keyframeContext->current->m_frame != keyframeContext->mutableFrame) {
                        compositeAction->GetActions().push_back(std::make_unique<MoveKeyframeAction>(keyframeContext->entity, keyframeContext->target, keyframeContext->current->m_frame, keyframeContext->mutableFrame));
                        if (ImGui::GetIO().KeyAlt) {
                            compositeAction->GetActions().push_back(std::make_unique<AddKeyframeAction>(keyframeContext->entity, keyframeContext->target, keyframeContext->current->m_frame, keyframeContext->current->m_value, keyframeContext->current->m_interpType));
                        }
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
    auto it = m_trackMetadata.find(child.get());
    if (std::end(m_trackMetadata) == it) {
        return false;
    }
    return it->second.expanded;
}

void EditorPanelAnimation::SetExpanded(std::shared_ptr<moth_ui::Node> child, bool expanded) {
    auto it = m_trackMetadata.find(child.get());
    if (std::end(m_trackMetadata) == it) {
        m_trackMetadata.insert(std::make_pair(child.get(), TrackMetadata{ child, expanded }));
    } else {
        it->second.expanded = expanded;
    }
}

void EditorPanelAnimation::SanitizeExtraData() {
    for (auto it = std::begin(m_trackMetadata); it != std::end(m_trackMetadata); /* skip */) {
        if (!it->second.ptr.lock()) {
            it = m_trackMetadata.erase(it);
        } else {
            ++it;
        }
    }
}
