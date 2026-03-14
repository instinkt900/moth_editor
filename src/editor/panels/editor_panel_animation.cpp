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
#include "../actions/delete_clip_action.h"
#include "../actions/add_event_action.h"
#include "../actions/delete_event_action.h"
#include "../actions/modify_event_action.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/nodes/group.h"
#include "moth_ui/layout/layout_entity_group.h"

#undef min
#undef max

using namespace moth_ui;

namespace {
    // Row backgrounds
    static constexpr ImU32 kColorRowOdd              = 0xFF3A3636;
    static constexpr ImU32 kColorRowEven             = 0xFF413D3D;
    static constexpr ImU32 kColorRowSpecial          = 0xFF3D3837; // clips and events rows

    // Text
    static constexpr ImU32 kColorTextPrimary         = 0xFFFFFFFF;
    static constexpr ImU32 kColorTextSecondary       = 0xFFBBBBBB; // frame numbers

    // Frame ruler
    static constexpr ImU32 kColorRulerTick           = 0xFF606060;

    // Clips
    static constexpr ImU32 kColorClip                = 0xFF13BDF3;
    static constexpr ImU32 kColorClipSelected        = 0xFF00CCAA;

    // Events and keyframes (shared colours)
    static constexpr ImU32 kColorElement             = 0xFFc4c4c4;
    static constexpr ImU32 kColorElementSelected     = 0xFFFFa2a2;

    // Scrollbar
    static constexpr ImU32 kColorScrollBarBg         = 0xFF505050;
    static constexpr ImU32 kColorScrollBarBgHover    = 0xFF606060;
    static constexpr ImU32 kColorScrollBarHandle     = 0xFF666666;
    static constexpr ImU32 kColorScrollBarHandleHover = 0xFFAAAAAA;

    // Box selection outline
    static constexpr ImU32 kColorSelectBox           = 0xFF00FFFF;

    // Draws a ghost of the element at its original position when Alt-dragging to duplicate.
    void DrawOriginalPositionGhost(ImDrawList* drawList, float trackStartOffsetX, float trackStartOffsetY,
                                   float rowHeight, int startFrame, int endFrame, float framePixelWidth,
                                   ImU32 color, float rounding) {
        float const startOffset = startFrame * framePixelWidth;
        float const endOffset = (endFrame + 1) * framePixelWidth;
        ImVec2 const boundsMin{ trackStartOffsetX + startOffset, trackStartOffsetY + 2.0f };
        ImVec2 const boundsMax{ trackStartOffsetX + endOffset, trackStartOffsetY + rowHeight - 2.0f };
        drawList->AddRectFilled(boundsMin, boundsMax, color, rounding);
    }

    // Reserves space in the scrolling panel so ImGui knows how big the content is.
    void AddScrollPanelItem(ImVec2 const& size_arg) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        const ImGuiID id = window->GetID("scroll_panel_item");
        ImVec2 size = ImGui::CalcItemSize(size_arg, 0.0f, 0.0f);
        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
        ImGui::ItemSize(size);
        ImGui::ItemAdd(bb, id);
    }
} // anonymous namespace

EditorPanelAnimation::EditorPanelAnimation(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Animation", visible, true) {
}

void EditorPanelAnimation::OnNewLayout() {
    m_currentFrame = 0;
    m_editorLayer.GetConfig().CurrentAnimationFrame = 0;
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
    // Static buffer reused each call; callers must consume before next call.
    static std::string labelBuffer;
    labelBuffer = fmt::format("{}: {}", index, GetEntityLabel(child->GetLayoutEntity()));
    return labelBuffer.c_str();
}

char const* EditorPanelAnimation::GetTrackLabel(AnimationTrack::Target target) const {
    // Static buffer reused each call; callers must consume before next call.
    static std::string labelBuffer;
    labelBuffer = magic_enum::enum_name(target);
    return labelBuffer.c_str();
}

// ---------------------------------------------------------------------------
// Selection management
// ---------------------------------------------------------------------------

void EditorPanelAnimation::ClearSelections() {
    m_selections.clear();
}

void EditorPanelAnimation::PerformOrCompositeAction(std::vector<std::unique_ptr<IEditorAction>> actions) {
    if (actions.empty()) {
        return;
    }
    if (actions.size() == 1) {
        m_editorLayer.PerformEditAction(std::move(actions[0]));
    } else {
        auto composite = std::make_unique<CompositeAction>();
        auto& target = composite->GetActions();
        target.insert(std::end(target), std::make_move_iterator(std::begin(actions)), std::make_move_iterator(std::end(actions)));
        m_editorLayer.PerformEditAction(std::move(composite));
    }
}

void EditorPanelAnimation::DeleteSelections() {
    if (m_selections.empty()) {
        return;
    }

    auto entity = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity());

    std::vector<std::unique_ptr<IEditorAction>> actions;
    for (auto& context : m_selections) {
        if (auto clipCtx = std::get_if<ClipContext>(&context)) {
            actions.push_back(std::make_unique<DeleteClipAction>(entity, *clipCtx->clip));
        } else if (auto eventCtx = std::get_if<EventContext>(&context)) {
            actions.push_back(std::make_unique<DeleteEventAction>(entity, *eventCtx->event));
        } else if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
            actions.push_back(std::make_unique<DeleteKeyframeAction>(kfCtx->entity, kfCtx->target, kfCtx->current->m_frame, kfCtx->current->m_value));
        }
    }

    PerformOrCompositeAction(std::move(actions));
    ClearSelections();
    m_editorLayer.Refresh();
}

// --- Clip selection ---

void EditorPanelAnimation::SelectClip(moth_ui::AnimationClip* clip) {
    if (!IsClipSelected(clip)) {
        ClipContext context;
        context.clip = clip;
        context.mutableValue = *clip;
        m_selections.push_back(context);
    }
}

void EditorPanelAnimation::DeselectClip(moth_ui::AnimationClip* clip) {
    auto const it = ranges::find_if(m_selections, [&](auto const& context) {
        if (auto clipCtx = std::get_if<ClipContext>(&context)) {
            return clipCtx->clip == clip;
        }
        return false;
    });
    if (it != std::end(m_selections)) {
        m_selections.erase(it);
    }
}

bool EditorPanelAnimation::IsClipSelected(moth_ui::AnimationClip* clip) {
    return GetSelectedClipContext(clip) != nullptr;
}

ClipContext* EditorPanelAnimation::GetSelectedClipContext(moth_ui::AnimationClip* clip) {
    auto const it = ranges::find_if(m_selections, [&](auto const& context) {
        if (auto clipCtx = std::get_if<ClipContext>(&context)) {
            return clipCtx->clip == clip;
        }
        return false;
    });
    if (it != std::end(m_selections)) {
        return std::get_if<ClipContext>(&(*it));
    }
    return nullptr;
}

// --- Event selection ---

void EditorPanelAnimation::SelectEvent(moth_ui::AnimationEvent* event) {
    if (!IsEventSelected(event)) {
        EventContext context;
        context.event = event;
        context.mutableValue = *event;
        m_selections.push_back(context);
    }
}

void EditorPanelAnimation::DeselectEvent(moth_ui::AnimationEvent* event) {
    auto const it = ranges::find_if(m_selections, [&](auto const& context) {
        if (auto eventCtx = std::get_if<EventContext>(&context)) {
            return eventCtx->event == event;
        }
        return false;
    });
    if (it != std::end(m_selections)) {
        m_selections.erase(it);
    }
}

bool EditorPanelAnimation::IsEventSelected(moth_ui::AnimationEvent* event) {
    return GetSelectedEventContext(event) != nullptr;
}

EventContext* EditorPanelAnimation::GetSelectedEventContext(moth_ui::AnimationEvent* event) {
    auto const it = ranges::find_if(m_selections, [&](auto const& context) {
        if (auto eventCtx = std::get_if<EventContext>(&context)) {
            return eventCtx->event == event;
        }
        return false;
    });
    if (it != std::end(m_selections)) {
        return std::get_if<EventContext>(&(*it));
    }
    return nullptr;
}

// --- Keyframe selection ---

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
    auto const it = ranges::find_if(m_selections, [&](auto const& context) {
        if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
            return kfCtx->entity == entity && kfCtx->target == target && kfCtx->current->m_frame == frameNo;
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
    auto const it = ranges::find_if(m_selections, [&](auto const& context) {
        if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
            return kfCtx->entity == entity && kfCtx->target == target && kfCtx->current->m_frame == frameNo;
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
        auto kfCtx = std::get_if<KeyframeContext>(&context);
        if (!kfCtx || (kfCtx->entity != entity || kfCtx->current->m_frame != frameNo)) {
            it = m_selections.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// Row layout
// ---------------------------------------------------------------------------

EditorPanelAnimation::RowDimensions EditorPanelAnimation::AddRow(char const* label, RowOptions const& rowOptions) {
    RowDimensions resultDimensions;

    ImVec2 const rowMin = m_scrollingPanelBounds.Min + ImVec2{ 0.0f, m_rowHeight * m_rowCounter };
    ImVec2 const rowMax = rowMin + ImVec2{ m_scrollingPanelBounds.GetWidth(), m_rowHeight };
    ImRect const rowBounds{ rowMin, rowMax };
    ImU32 const rowColor = rowOptions.colorOverride.has_value() ? rowOptions.colorOverride.value() : (((m_rowCounter % 2) == 0) ? kColorRowOdd : kColorRowEven);

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

    if (label != nullptr) {
        ImVec2 const labelMin{ cursorPos, rowMin.y };
        ImVec2 const labelMax{ rowMin.x + m_labelColumnWidth, rowMax.y };
        ImRect const labelBounds{ labelMin, labelMax };
        ImVec2 const textPos = labelMin + ImVec2(rowOptions.indented ? 50.0f : 3.0f, 0.0f);
        m_drawList->AddText(textPos, kColorTextPrimary, label);
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

// ---------------------------------------------------------------------------
// Frame number ribbon (top ruler)
// ---------------------------------------------------------------------------

void EditorPanelAnimation::DrawFrameNumberRibbon() {

    ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
    ImGui::InvisibleButton("frameRibbon_", ImVec2(canvasSize.x, m_rowHeight));
    ImVec2 aMin = ImGui::GetItemRectMin();
    ImVec2 aMax = ImGui::GetItemRectMax();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Compute a nice major tick step that keeps labels from overlapping.
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

        if (px >= trackMin.x && px <= trackMax.x) {
            ImVec2 const start{ px, trackMin.y + tickYOffset };
            ImVec2 const end{ px, trackMax.y - 1 };
            drawList->AddLine(start, end, kColorRulerTick, 1);

            if (majorTick) {
                char frameLabel[32];
                ImFormatString(frameLabel, IM_ARRAYSIZE(frameLabel), "%d", i);
                drawList->AddText(ImVec2(px + 3.f, trackMin.y), kColorTextSecondary, frameLabel);
            }
        }
    }

    // Clicking on the ribbon grabs the current-frame indicator.
    ImGuiIO const& io = ImGui::GetIO();
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && trackBounds.Contains(io.MousePos)) {
        m_grabbedCurrentFrame = true;
    }

    if (m_grabbedCurrentFrame) {
        m_currentFrame = MousePosToFrame(io.MousePos.x, trackBounds.Min.x);
        m_currentFrame = std::clamp(m_currentFrame, m_minFrame, m_maxFrame);

        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_grabbedCurrentFrame = false;
        }
    }

    // Calculate cursor rect (drawn later so it renders on top of tracks).
    float const cursorLeft = trackBounds.Min.x + (m_currentFrame - m_minFrame) * m_framePixelWidth;
    m_cursorRect.Min = { cursorLeft, trackBounds.Min.y };
    m_cursorRect.Max = { cursorLeft + m_framePixelWidth + 1.0f, trackBounds.Min.y + canvasSize.y - m_horizontalScrollbarHeight };
}

// ---------------------------------------------------------------------------
// Clip popup (right-click context menu on clips row)
// ---------------------------------------------------------------------------

bool EditorPanelAnimation::DrawClipPopup() {
    if (ImGui::BeginPopup(ClipPopupName)) {
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

            static char nameBuf[1024];
            ImFormatString(nameBuf, IM_ARRAYSIZE(nameBuf), "%s", m_pendingClipEdit->mutableValue.m_name.c_str());
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                m_pendingClipEdit->mutableValue.m_name = std::string(nameBuf);
            }

            ImGui::InputFloat("FPS", &m_pendingClipEdit->mutableValue.m_fps);

            std::string preview = std::string(magic_enum::enum_name(m_pendingClipEdit->mutableValue.m_loopType));
            if (ImGui::BeginCombo("Loop Type", preview.c_str())) {
                for (size_t n = 0; n < magic_enum::enum_count<AnimationClip::LoopType>(); n++) {
                    auto const enumValue = magic_enum::enum_value<AnimationClip::LoopType>(n);
                    std::string enumName = std::string(magic_enum::enum_name(enumValue));
                    const bool isSelected = m_pendingClipEdit->mutableValue.m_loopType == enumValue;
                    if (ImGui::Selectable(enumName.c_str(), isSelected)) {
                        m_pendingClipEdit->mutableValue.m_loopType = enumValue;
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::EndMenu();
        }
        ImGui::EndPopup();
        return true;
    }

    // Popup just closed — commit any pending edits if the clip still exists and changed.
    if (m_pendingClipEdit.has_value()) {
        auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
        auto const& clips = groupEntity->m_clips;
        bool const stillExists = std::any_of(clips.begin(), clips.end(), [&](auto const& c) { return c.get() == m_pendingClipEdit->reference; });
        if (stillExists && m_pendingClipEdit->HasChanged()) {
            auto action = std::make_unique<ModifyClipAction>(groupEntity, *m_pendingClipEdit->reference, m_pendingClipEdit->mutableValue);
            m_editorLayer.PerformEditAction(std::move(action));
        }
        m_pendingClipEdit.reset();
    }

    return false;
}

// ---------------------------------------------------------------------------
// Clip row
// ---------------------------------------------------------------------------

void EditorPanelAnimation::DrawClipRow() {
    RowDimensions const rowDimensions = AddRow("Clips", RowOptions().ColorOverride(kColorRowSpecial));

    ImGuiIO const& io = ImGui::GetIO();

    bool const popupShown = DrawClipPopup();

    // Cancel pending selection clear if we click in a popup.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_clickConsumed = true;
    }

    m_drawList->PushClipRect(rowDimensions.trackBounds.Min, rowDimensions.trackBounds.Max, true);

    float const trackStartOffsetX = rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset;
    float const trackStartOffsetY = rowDimensions.trackBounds.Min.y;

    auto& animationClips = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity())->m_clips;
    for (auto&& clip : animationClips) {
        bool selected = IsClipSelected(clip.get());

        auto& clipValues = (m_mouseDragging && selected) ? GetSelectedClipContext(clip.get())->mutableValue : *clip;

        float const clipStartOffset = clipValues.m_startFrame * m_framePixelWidth;
        float const clipEndOffset = (clipValues.m_endFrame + 1) * m_framePixelWidth;
        ImVec2 const clipBoundsMin{ trackStartOffsetX + clipStartOffset, trackStartOffsetY + 2.0f };
        ImVec2 const clipBoundsMax{ trackStartOffsetX + clipEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };

        if (m_boxSelecting && ImRect{ clipBoundsMin, clipBoundsMax }.Overlaps(m_selectBox)) {
            m_pendingClipBoxSelections.push_back(clip.get());
            selected = true;
        }

        unsigned int const slotColor = selected ? kColorClipSelected : kColorClip;
        m_drawList->AddRectFilled(clipBoundsMin, clipBoundsMax, slotColor, 2.0f);

        // Alt-drag duplicates: show the original clip in place.
        if (io.KeyAlt) {
            DrawOriginalPositionGhost(m_drawList, trackStartOffsetX, trackStartOffsetY, m_rowHeight,
                                      clip->m_startFrame, clip->m_endFrame, m_framePixelWidth, slotColor, 2.0f);
        }

        // The clip has three interactive regions: left edge, right edge, and center body.
        // Left/right edges resize; center moves the whole clip.
        float const clipEdgeWidth = m_framePixelWidth / 2;
        ImRect const hitRegions[3] = {
            ImRect{ clipBoundsMin, ImVec2{ clipBoundsMin.x + clipEdgeWidth, clipBoundsMax.y } },
            ImRect{ ImVec2{ clipBoundsMax.x - clipEdgeWidth, clipBoundsMin.y }, clipBoundsMax },
            ImRect{ clipBoundsMin, clipBoundsMax }
        };

        unsigned int const hoverColor[] = { kColorTextPrimary, kColorTextPrimary, slotColor };
        if (!m_mouseDragging && m_scrollingPanelBounds.Contains(io.MousePos)) {
            // Draw hover highlight (check center last so edges take priority).
            for (int i = 2; i >= 0; --i) {
                if (hitRegions[i].Contains(io.MousePos)) {
                    m_drawList->AddRectFilled(hitRegions[i].Min, hitRegions[i].Max, hoverColor[i], 2.0f);
                }
            }

            if (io.MousePos.x >= rowDimensions.trackBounds.Min.x && io.MousePos.x <= rowDimensions.trackBounds.Max.x) {
                // Check hit regions for click (edges first so they take priority over center).
                for (int j = 0; j < 3; j++) {
                    if (!m_mouseInScrollArea || !hitRegions[j].Contains(io.MousePos))
                        continue;

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                        if (!IsClipSelected(clip.get())) {
                            if ((io.KeyMods & ImGuiModFlags_Ctrl) == 0) {
                                ClearSelections();
                            }
                        }

                        SelectClip(clip.get());
                        m_clickConsumed = true;

                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            m_mouseDragging = true;
                            m_mouseDragStartX = io.MousePos.x;
                            static constexpr int kHandleForRegion[3] = { kClipHandleLeft, kClipHandleRight, kClipHandleCenter };
                            m_clipDragHandle = kHandleForRegion[j];
                            break;
                        }
                    }
                }
            }
        }

        ImVec2 const textSize = ImGui::CalcTextSize(clip->m_name.c_str());
        ImVec2 const textPos(clipBoundsMin.x + (clipBoundsMax.x - clipBoundsMin.x - textSize.x) / 2, clipBoundsMin.y + (clipBoundsMax.y - clipBoundsMin.y - textSize.y) / 2);
        m_drawList->AddText(textPos, kColorTextPrimary, clip->m_name.c_str());
    }

    m_drawList->PopClipRect();

    // Right-click opens clip context menu.
    if (m_mouseInScrollArea && rowDimensions.trackBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        m_clickedFrame = MousePosToFrame(io.MousePos.x, rowDimensions.trackBounds.Min.x);
        ImGui::OpenPopup(ClipPopupName);
    }
}

// ---------------------------------------------------------------------------
// Event popup (right-click context menu on events row)
// ---------------------------------------------------------------------------

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
            static char nameBuf[1024];
            if (!m_pendingEventEdit.has_value()) {
                m_pendingEventEdit = EditContext<AnimationEvent>(eventContext->event);
                ImFormatString(nameBuf, IM_ARRAYSIZE(nameBuf), "%s", eventContext->event->m_name.c_str());
            }

            if (ImGui::InputText("Event Name", nameBuf, sizeof(nameBuf))) {
                m_pendingEventEdit->mutableValue.m_name = std::string(nameBuf);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
        return true;
    }

    // Popup just closed — commit any pending edits if the event still exists and changed.
    if (m_pendingEventEdit.has_value()) {
        auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
        auto const& events = groupEntity->m_events;
        bool const stillExists = std::any_of(events.begin(), events.end(), [&](auto const& e) { return e.get() == m_pendingEventEdit->reference; });
        if (stillExists && m_pendingEventEdit->HasChanged()) {
            auto action = std::make_unique<ModifyEventAction>(groupEntity, *m_pendingEventEdit->reference, m_pendingEventEdit->mutableValue);
            m_editorLayer.PerformEditAction(std::move(action));
        }
        m_pendingEventEdit.reset();
    }

    return false;
}

// ---------------------------------------------------------------------------
// Events row
// ---------------------------------------------------------------------------

void EditorPanelAnimation::DrawEventsRow() {
    ImGuiIO const& io = ImGui::GetIO();
    RowDimensions const rowDimensions = AddRow("Events", RowOptions().ColorOverride(kColorRowSpecial));

    bool const popupShown = DrawEventPopup();

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_clickConsumed = true;
    }

    m_drawList->PushClipRect(rowDimensions.trackBounds.Min, rowDimensions.trackBounds.Max, true);

    float const trackStartOffsetX = rowDimensions.trackBounds.Min.x + rowDimensions.trackOffset;
    float const trackStartOffsetY = rowDimensions.trackBounds.Min.y;

    auto& animationEvents = std::static_pointer_cast<LayoutEntityGroup>(m_group->GetLayoutEntity())->m_events;
    for (auto& event : animationEvents) {
        bool selected = IsEventSelected(event.get());

        auto& eventValues = (m_mouseDragging && selected) ? GetSelectedEventContext(event.get())->mutableValue : *event;

        float const eventStartOffset = eventValues.m_frame * m_framePixelWidth;
        float const eventEndOffset = (eventValues.m_frame + 1) * m_framePixelWidth;
        ImVec2 const eventBoundsMin{ trackStartOffsetX + eventStartOffset, trackStartOffsetY + 2.0f };
        ImVec2 const eventBoundsMax{ trackStartOffsetX + eventEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
        ImRect const eventBounds{ eventBoundsMin, eventBoundsMax };

        if (m_boxSelecting && eventBounds.Overlaps(m_selectBox)) {
            m_pendingEventBoxSelections.push_back(event.get());
            selected = true;
        }

        unsigned int const slotColor = selected ? kColorElementSelected : kColorElement;
        m_drawList->AddRectFilled(eventBoundsMin, eventBoundsMax, slotColor, 2.0f);

        if (eventBounds.Contains(io.MousePos)) {
            ImGui::BeginTooltip();
            ImGui::Text("Event \"%s\"", eventValues.m_name.c_str());
            ImGui::EndTooltip();
        }

        // Alt-drag duplicates: show the original event in place.
        if (io.KeyAlt) {
            DrawOriginalPositionGhost(m_drawList, trackStartOffsetX, trackStartOffsetY, m_rowHeight,
                                      event->m_frame, event->m_frame, m_framePixelWidth, slotColor, 2.0f);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (m_mouseInScrollArea && eventBounds.Contains(io.MousePos)) {
                if ((io.KeyMods & ImGuiModFlags_Ctrl) == 0) {
                    ClearSelections();
                }

                SelectEvent(event.get());
                m_clickConsumed = true;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    m_mouseDragging = true;
                    m_mouseDragStartX = io.MousePos.x;
                }
            }
        }
    }

    m_drawList->PopClipRect();

    // Right-click opens event context menu.
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        if (m_mouseInScrollArea && rowDimensions.trackBounds.Contains(io.MousePos)) {
            m_clickedFrame = MousePosToFrame(io.MousePos.x, rowDimensions.trackBounds.Min.x);
            ImGui::OpenPopup(EventPopupName);
        }
    }
}

// ---------------------------------------------------------------------------
// Keyframe popup (right-click context menu on keyframe tracks)
// ---------------------------------------------------------------------------

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
                // Clicked on a specific sub-track: add one keyframe.
                auto const currentValue = trackPtr->GetValueAtFrame(static_cast<float>(m_clickedFrame));
                std::unique_ptr<IEditorAction> action = std::make_unique<AddKeyframeAction>(childEntity, m_clickedChildTarget, m_clickedFrame, currentValue, moth_ui::InterpType::Linear);
                action->Do();
                m_editorLayer.AddEditAction(std::move(action));
            } else {
                // Clicked on the collapsed main track: add keyframes on all continuous targets.
                std::unique_ptr<CompositeAction> compositeAction = std::make_unique<CompositeAction>();
                for (auto&& target : AnimationTrack::ContinuousTargets) {
                    trackPtr = childTracks.at(target).get();
                    if (nullptr == trackPtr->GetKeyframe(m_clickedFrame)) {
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
            // Determine whether the selected keyframes share the same interp type.
            bool multipleInterps = false;
            moth_ui::InterpType selectedInterp = moth_ui::InterpType::Unknown;
            for (auto&& context : m_selections) {
                if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
                    if (kfCtx->current->m_interpType != selectedInterp) {
                        if (selectedInterp == moth_ui::InterpType::Unknown) {
                            selectedInterp = kfCtx->current->m_interpType;
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
                        if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
                            auto action = MakeChangeValueAction(kfCtx->current->m_interpType, kfCtx->current->m_interpType, interpType, nullptr);
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

// ---------------------------------------------------------------------------
// Child track (keyframe rows for one child entity)
// ---------------------------------------------------------------------------

void EditorPanelAnimation::DrawChildTrack(int childIndex, std::shared_ptr<Node> child) {
    ImGuiIO const& io = ImGui::GetIO();

    auto const childEntity = child->GetLayoutEntity();
    auto& childTracks = childEntity->m_tracks;
    bool const entitySelected = m_editorLayer.IsSelected(child);
    bool expanded = IsExpanded(child);

    RowOptions rowOptions;
    rowOptions.expandable = true;
    rowOptions.expanded = expanded;
    if (entitySelected) {
        rowOptions.colorOverride = 0x55FF3333;
    }
    RowDimensions const rowDimensions = AddRow(GetChildLabel(childIndex), rowOptions);

    // Toggle expand/collapse.
    if (m_mouseInScrollArea && rowDimensions.buttonBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        expanded = !expanded;
    }
    SetExpanded(child, expanded);

    // Click on label to select the entity in the editor.
    if (m_mouseInScrollArea && rowDimensions.labelBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!io.KeyCtrl) {
            m_editorLayer.ClearSelection();
        }
        if (m_editorLayer.IsSelected(child)) {
            m_editorLayer.RemoveSelection(child);
        } else {
            m_editorLayer.AddSelection(child);
        }
    }

    for (auto& [target, track] : childTracks) {
        ImRect subTrackBounds = rowDimensions.trackBounds;

        if (expanded) {
            RowDimensions const subRowDimensions = AddRow(GetTrackLabel(target), RowOptions().Indented(true));
            subTrackBounds = subRowDimensions.trackBounds;
        }

        m_drawList->PushClipRect(subTrackBounds.Min, subTrackBounds.Max, true);

        float const trackStartOffsetX = subTrackBounds.Min.x + rowDimensions.trackOffset;
        float const trackStartOffsetY = subTrackBounds.Min.y;

        for (auto& keyframe : track->Keyframes()) {
            bool selected = IsKeyframeSelected(childEntity, target, keyframe->m_frame);

            int const frameNumber = (m_mouseDragging && selected) ? GetSelectedKeyframeContext(childEntity, target, keyframe->m_frame)->mutableFrame : keyframe->m_frame;

            float const frameStartOffset = frameNumber * m_framePixelWidth + 1.0f;
            float const frameEndOffset = (frameNumber + 1) * m_framePixelWidth;
            ImVec2 const frameBoundsMin{ trackStartOffsetX + frameStartOffset, trackStartOffsetY + 2.0f };
            ImVec2 const frameBoundsMax{ trackStartOffsetX + frameEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
            ImRect const frameBounds{ frameBoundsMin, frameBoundsMax };

            if (m_boxSelecting && frameBounds.Overlaps(m_selectBox)) {
                m_pendingBoxSelections.push_back(KeyframeContext{ childEntity, target, keyframe->m_frame });
                selected = true;
            }

            unsigned int const slotColor = selected ? kColorElementSelected : kColorElement;
            m_drawList->AddRectFilled(frameBoundsMin, frameBoundsMax, slotColor, 0.0f);

            // Alt-drag duplicates: show the original keyframe in place.
            if (io.KeyAlt) {
                float const origStartOffset = keyframe->m_frame * m_framePixelWidth + 1.0f;
                float const origEndOffset = (keyframe->m_frame + 1) * m_framePixelWidth;
                ImVec2 const origMin{ trackStartOffsetX + origStartOffset, trackStartOffsetY + 2.0f };
                ImVec2 const origMax{ trackStartOffsetX + origEndOffset, trackStartOffsetY + m_rowHeight - 2.0f };
                m_drawList->AddRectFilled(origMin, origMax, slotColor, 0.0f);
            }

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (m_mouseInScrollArea && frameBounds.Contains(io.MousePos)) {
                    if (!IsKeyframeSelected(childEntity, target, keyframe->m_frame)) {
                        if ((io.KeyMods & ImGuiModFlags_Ctrl) == 0) {
                            if (expanded) {
                                ClearSelections();
                            } else {
                                // When collapsed, keep sibling-track keyframes at the same frame selected
                                // so the user can move all tracks at once.
                                FilterKeyframeSelections(childEntity, keyframe->m_frame);
                            }
                        }
                    }

                    SelectKeyframe(childEntity, target, keyframe->m_frame);
                    m_clickConsumed = true;

                    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        m_mouseDragging = true;
                        m_mouseDragStartX = io.MousePos.x;
                    }
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    if (m_mouseInScrollArea && subTrackBounds.Contains(io.MousePos)) {
                        m_clickedChildIdx = childIndex;
                        m_clickedChildTarget = expanded ? target : AnimationTrack::Target::Unknown;
                        m_clickedFrame = MousePosToFrame(io.MousePos.x, subTrackBounds.Min.x);
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

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && popupShown) {
        m_clickConsumed = true;
    }

    int childIndex = 0;
    for (auto& child : m_group->GetChildren()) {
        DrawChildTrack(childIndex, child);
        ++childIndex;
    }
}

// ---------------------------------------------------------------------------
// Horizontal scroll bar
// ---------------------------------------------------------------------------

void EditorPanelAnimation::DrawHorizScrollBar() {
    ImGuiIO const& io = ImGui::GetIO();
    float const scrollingPanelWidth = m_scrollingPanelBounds.GetWidth();
    ImGui::InvisibleButton("scrollBar", { scrollingPanelWidth, m_horizontalScrollbarHeight - 4.0f });
    ImVec2 const elementBoundsMin = ImGui::GetItemRectMin();
    ImVec2 const elementBoundsMax = ImGui::GetItemRectMax();
    ImRect const elementBounds{ elementBoundsMin, elementBoundsMax };

    // Scroll track (the full-width background).
    ImVec2 const scrollTrackMin{ elementBounds.Min.x + m_labelColumnWidth, elementBounds.Min.y - 2 };
    ImVec2 const scrollTrackMax{ elementBounds.Min.x + scrollingPanelWidth, elementBounds.Max.y - 1 };
    ImRect const scrollTrackBounds{ scrollTrackMin, scrollTrackMax };

    // Scroll handle (the draggable bar within the track).
    ImVec2 const scrollBarMin = { scrollTrackBounds.Min.x + scrollTrackBounds.GetWidth() * m_hScrollFactors.x, elementBounds.Min.y };
    ImVec2 const scrollBarMax = { scrollTrackBounds.Min.x + scrollTrackBounds.GetWidth() * m_hScrollFactors.y, elementBounds.Max.y - 2 };
    ImRect const scrollBarBounds{ scrollBarMin, scrollBarMax };

    // Resize handles on left and right edges of the scroll handle.
    float const handleWidth = 14.0f;
    ImRect const barHandleLeft{ scrollBarMin, { scrollBarMin.x + handleWidth, scrollBarMax.y } };
    ImRect const barHandleRight{ { scrollBarMax.x - handleWidth, scrollBarMin.y }, scrollBarMax };

    bool const mouseInScrollBar = scrollTrackBounds.Contains(io.MousePos);
    bool const mouseInLeftHandle = barHandleLeft.Contains(io.MousePos);
    bool const mouseInRightHandle = barHandleRight.Contains(io.MousePos);

    m_drawList->AddRectFilled(scrollBarMin, scrollBarMax, (mouseInScrollBar || m_hScrollGrabbedBar) ? kColorScrollBarBgHover : kColorScrollBarBg, 6);
    m_drawList->AddRectFilled(barHandleLeft.Min, barHandleLeft.Max, (mouseInLeftHandle || m_hScrollGrabbedLeft) ? kColorScrollBarHandleHover : kColorScrollBarHandle, 6);
    m_drawList->AddRectFilled(barHandleRight.Min, barHandleRight.Max, (mouseInRightHandle || m_hScrollGrabbedRight) ? kColorScrollBarHandleHover : kColorScrollBarHandle, 6);

    if (m_hScrollGrabbedRight) {
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedRight = false;
        } else if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
            float const delta = io.MouseDelta.x / scrollTrackBounds.GetWidth();
            m_hScrollFactors.y = std::clamp(m_hScrollFactors.y + delta, 0.0f, 1.0f);
            m_maxFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.y);
        }
    } else if (m_hScrollGrabbedLeft) {
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedLeft = false;
        } else if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
            float const delta = io.MouseDelta.x / scrollTrackBounds.GetWidth();
            m_hScrollFactors.x = std::clamp(m_hScrollFactors.x + delta, 0.0f, 1.0f);
            m_minFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.x);
        }
    } else if (m_hScrollGrabbedBar) {
        if (!io.MouseDown[ImGuiMouseButton_Left]) {
            m_hScrollGrabbedBar = false;
        } else if (fabsf(io.MouseDelta.x) > FLT_EPSILON) {
            float const delta = io.MouseDelta.x / scrollTrackBounds.GetWidth();
            ImVec2 const newScrollFactors = m_hScrollFactors + ImVec2{ delta, delta };
            if (newScrollFactors.x < 0.0f) {
                float const clampedMove = m_hScrollFactors.x;
                m_hScrollFactors.x = 0.0f;
                m_hScrollFactors.y -= clampedMove;
            } else if (newScrollFactors.y > 1.0f) {
                float const clampedMove = m_hScrollFactors.y - 1.0f;
                m_hScrollFactors.x += clampedMove;
                m_hScrollFactors.y = 1.0f;
            } else {
                m_hScrollFactors.x = std::clamp(m_hScrollFactors.x + delta, 0.0f, 1.0f);
                m_hScrollFactors.y = std::clamp(m_hScrollFactors.y + delta, 0.0f, 1.0f);
            }
            m_minFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.x);
            m_maxFrame = static_cast<int>(m_totalFrames * m_hScrollFactors.y);
        }
    } else {
        // Not currently grabbing anything — check for new grabs.
        // Handle checks come first so they take priority over the bar body.
        if (!m_hScrollGrabbedLeft && mouseInLeftHandle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_hScrollGrabbedLeft = true;
        } else if (!m_hScrollGrabbedRight && mouseInRightHandle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_hScrollGrabbedRight = true;
        } else if (scrollBarBounds.Contains(io.MousePos) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_grabbedCurrentFrame) {
            m_hScrollGrabbedBar = true;
        }
    }

    // +1 so that the max frame is visible within the track area.
    m_framePixelWidth = scrollTrackBounds.GetWidth() / (m_maxFrame - m_minFrame + 1);
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

int EditorPanelAnimation::CalcNumRows() const {
    int count = 3; // frame ribbon + clips + events
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
        char frameLabel[512];
        ImFormatString(frameLabel, IM_ARRAYSIZE(frameLabel), "%d", m_currentFrame);
        m_drawList->AddText(ImVec2(m_cursorRect.Min.x + 3.0f, m_cursorRect.Min.y), kColorTextPrimary, frameLabel);
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

// Adapted from https://github.com/ocornut/imgui/issues/3379 (thanks ocornut).
// Scrolls the panel when the user middle-click-drags on empty space.
void EditorPanelAnimation::ScrollWhenDraggingOnVoid(const ImVec2& delta, ImGuiMouseButton mouse_button) {
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiWindow* window = g.CurrentWindow;
    bool hovered = false;
    bool held = false;
    ImGuiID id = window->GetID("##scrolldraggingoverlay");
    ImGui::KeepAliveID(id);
    ImGuiButtonFlags button_flags = (mouse_button == 0) ? ImGuiButtonFlags_MouseButtonLeft : (mouse_button == 1) ? ImGuiButtonFlags_MouseButtonRight
                                                                                                                 : ImGuiButtonFlags_MouseButtonMiddle;

    static float xDeltaAcc = 0;
    if (g.HoveredId == 0) {
        ImGui::ButtonBehavior(window->Rect(), id, &hovered, &held, button_flags);
    }
    if (held && delta.x != 0.0f) {
        xDeltaAcc += delta.x;
        int frameDelta = static_cast<int>(xDeltaAcc / m_framePixelWidth);
        xDeltaAcc -= static_cast<float>(frameDelta * m_framePixelWidth);
        if (frameDelta != 0) {
            // Clamp so we don't scroll past the edges.
            if (frameDelta < 0) {
                frameDelta = -std::min(-frameDelta, m_minFrame);
            } else {
                frameDelta = std::min(frameDelta, m_totalFrames - m_maxFrame);
            }
            m_minFrame += frameDelta;
            m_maxFrame += frameDelta;
            m_hScrollFactors.x = m_minFrame / static_cast<float>(m_totalFrames);
            m_hScrollFactors.y = m_maxFrame / static_cast<float>(m_totalFrames);
        }
    }
    if (held && delta.y != 0.0f) {
        ImGui::SetScrollY(window, window->Scroll.y + delta.y);
    }

    if (!held) {
        xDeltaAcc = 0;
    }
}

// ---------------------------------------------------------------------------
// Main widget entry point
// ---------------------------------------------------------------------------

void EditorPanelAnimation::DrawWidget() {
    m_group = m_editorLayer.GetRoot();
    SanitizeExtraData();

    m_rowCounter = 0;

    m_minFrame = std::max(0, m_minFrame);
    m_currentFrame = std::min(m_totalFrames, m_currentFrame);

    // Persist frame range to layout file so it survives editor restarts.
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

    // Vertical scrolling panel that contains all track rows.
    ImVec2 const canvasSize = ImGui::GetContentRegionAvail();
    int const targetRowCount = CalcNumRows();
    float const panelHeight = m_rowHeight * targetRowCount;
    ImGui::BeginChildFrame(889, ImVec2{ canvasSize.x, canvasSize.y - m_horizontalScrollbarHeight });
    ImRect const frameRect = { ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetWindowSize() };
    m_mouseInScrollArea = frameRect.Contains(ImGui::GetMousePos());
    AddScrollPanelItem(ImVec2(canvasSize.x - m_verticalScrollbarWidth, static_cast<float>(panelHeight)));
    m_scrollingPanelBounds.Min = ImGui::GetItemRectMin();
    m_scrollingPanelBounds.Max = ImGui::GetItemRectMax();

    // Clicking on blank track area clears the selection.
    // Clickable elements set m_clickConsumed = true to prevent this.
    ImRect trackAreaBounds;
    trackAreaBounds.Min = { m_scrollingPanelBounds.Min.x + m_labelColumnWidth, m_scrollingPanelBounds.Min.y };
    trackAreaBounds.Max = m_scrollingPanelBounds.Max;

    bool const leftClickInTrackArea = trackAreaBounds.Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    m_clickConsumed = !leftClickInTrackArea;

    m_drawList = ImGui::GetWindowDrawList();

    m_pendingBoxSelections.clear();
    m_pendingClipBoxSelections.clear();
    m_pendingEventBoxSelections.clear();

    DrawClipRow();
    DrawEventsRow();
    DrawTrackRows();

    // Box selection: start tracking on unhandled click in the track area.
    if (m_mouseInScrollArea && !m_clickConsumed) {
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

            // Require a minimum drag area before activating box select,
            // so small clicks don't accidentally trigger it.
            if (m_selectBox.GetArea() > 30) {
                m_boxSelecting = true;
            }

            if (m_boxSelecting) {
                m_drawList->AddRect(m_selectBox.Min, m_selectBox.Max, kColorSelectBox);
            }
        }
    } else {
        m_boxSelectStarted = false;
        m_boxSelecting = false;

        // Commit pending box selections now that the mouse is released.
        for (auto& pending : m_pendingBoxSelections) {
            SelectKeyframe(pending.entity, pending.target, pending.mutableFrame);
        }
        for (auto* clip : m_pendingClipBoxSelections) {
            SelectClip(clip);
        }
        for (auto* event : m_pendingEventBoxSelections) {
            SelectEvent(event);
        }
    }

    ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
    ScrollWhenDraggingOnVoid(mouseDelta * -1, ImGuiMouseButton_Middle);

    ImGui::EndChildFrame();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    DrawCursor();

    DrawHorizScrollBar();

    m_drawList = nullptr;

    // Clear selections if nothing consumed the click and Ctrl isn't held.
    auto& io = ImGui::GetIO();
    if (!m_clickConsumed && !io.KeyCtrl) {
        ClearSelections();
    }

    UpdateMouseDragging();
}

// ---------------------------------------------------------------------------
// Mouse drag commit
// ---------------------------------------------------------------------------

void EditorPanelAnimation::CommitDragActions() {
    auto groupEntity = std::static_pointer_cast<moth_ui::LayoutEntityGroup>(m_group->GetLayoutEntity());
    std::vector<std::unique_ptr<IEditorAction>> actions;

    for (auto& context : m_selections) {
        if (auto clipCtx = std::get_if<ClipContext>(&context)) {
            bool const changed = clipCtx->clip->m_startFrame != clipCtx->mutableValue.m_startFrame || clipCtx->clip->m_endFrame != clipCtx->mutableValue.m_endFrame;
            if (changed) {
                actions.push_back(std::make_unique<ModifyClipAction>(groupEntity, *clipCtx->clip, clipCtx->mutableValue));
                // Alt-drag: duplicate the original clip at its old position.
                if (ImGui::GetIO().KeyAlt) {
                    actions.push_back(std::make_unique<AddClipAction>(groupEntity, *clipCtx->clip));
                }
            }
        } else if (auto eventCtx = std::get_if<EventContext>(&context)) {
            if (eventCtx->event->m_frame != eventCtx->mutableValue.m_frame) {
                actions.push_back(std::make_unique<ModifyEventAction>(groupEntity, *eventCtx->event, eventCtx->mutableValue));
                if (ImGui::GetIO().KeyAlt) {
                    actions.push_back(std::make_unique<AddEventAction>(groupEntity, eventCtx->event->m_frame, eventCtx->event->m_name));
                }
            }
        } else if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
            if (kfCtx->current->m_frame != kfCtx->mutableFrame) {
                actions.push_back(std::make_unique<MoveKeyframeAction>(kfCtx->entity, kfCtx->target, kfCtx->current->m_frame, kfCtx->mutableFrame));
                if (ImGui::GetIO().KeyAlt) {
                    actions.push_back(std::make_unique<AddKeyframeAction>(kfCtx->entity, kfCtx->target, kfCtx->current->m_frame, kfCtx->current->m_value, kfCtx->current->m_interpType));
                }
            }
        }
    }

    PerformOrCompositeAction(std::move(actions));
}

void EditorPanelAnimation::UpdateMouseDragging() {
    if (!m_mouseDragging) {
        return;
    }

    float const dragDelta = ImGui::GetMousePos().x - m_mouseDragStartX;
    int const frameDelta = static_cast<int>(dragDelta / m_framePixelWidth);

    if (frameDelta != 0) {
        for (auto& context : m_selections) {
            if (auto clipCtx = std::get_if<ClipContext>(&context)) {
                // Single clip: the drag handle determines whether we move the left edge, right edge, or both.
                if (m_selections.size() == 1) {
                    int& l = clipCtx->mutableValue.m_startFrame;
                    int& r = clipCtx->mutableValue.m_endFrame;
                    if (m_clipDragHandle & kClipHandleLeft)
                        l = clipCtx->clip->m_startFrame + frameDelta;
                    if (m_clipDragHandle & kClipHandleRight)
                        r = clipCtx->clip->m_endFrame + frameDelta;
                    if (l < 0) {
                        if (m_clipDragHandle & kClipHandleRight)
                            r -= l;
                        l = 0;
                    }
                    if (m_clipDragHandle & kClipHandleLeft && l > r)
                        l = r;
                    if (m_clipDragHandle & kClipHandleRight && r < l)
                        r = l;
                } else {
                    clipCtx->mutableValue.m_startFrame = clipCtx->clip->m_startFrame + frameDelta;
                    clipCtx->mutableValue.m_endFrame = clipCtx->clip->m_endFrame + frameDelta;
                }
            } else if (auto eventCtx = std::get_if<EventContext>(&context)) {
                eventCtx->mutableValue.m_frame = eventCtx->event->m_frame + frameDelta;
            } else if (auto kfCtx = std::get_if<KeyframeContext>(&context)) {
                kfCtx->mutableFrame = kfCtx->current->m_frame + frameDelta;
            }
        }
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        m_mouseDragging = false;
        CommitDragActions();
    }
}

// ---------------------------------------------------------------------------
// Track metadata (expand/collapse state per child node)
// ---------------------------------------------------------------------------

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
    // Purge entries for nodes that have been destroyed.
    for (auto it = std::begin(m_trackMetadata); it != std::end(m_trackMetadata); /* skip */) {
        if (!it->second.ptr.lock()) {
            it = m_trackMetadata.erase(it);
        } else {
            ++it;
        }
    }
}
