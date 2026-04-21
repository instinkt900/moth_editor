#include "common.h"
#include "editor_panel_properties.h"

#include "../properties_elements.h"
#include "../actions/editor_action.h"
#include "../imgui_ext_inspect.h"
#include "../utils.h"
#include "../editor_layer.h"

#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/layout/layout_entity_ref.h"
#include "moth_ui/layout/layout_entity_flipbook.h"
#include "../actions/add_discrete_keyframe_action.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/nodes/node_rect.h"
#include "moth_ui/nodes/node_image.h"
#include "moth_ui/nodes/node_text.h"
#include "moth_ui/nodes/node_flipbook.h"
#include "moth_ui/nodes/group.h"
#include "moth_ui/context.h"

#include <nfd.h>

#undef min
#undef max

EditorPanelProperties::EditorPanelProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Properties", visible, true) {
}

void EditorPanelProperties::OnLayoutLoaded() {
    m_lastSelection = nullptr;
}

bool EditorPanelProperties::BeginPanel() {
    const auto& selection = m_editorLayer.GetSelection();
    if (selection.empty()) {
        m_currentSelection = m_editorLayer.GetRoot();
    } else {
        m_currentSelection = *std::begin(selection);
    }

    bool ret = EditorPanel::BeginPanel();
    ImGui::PushID(m_currentSelection.get());
    return ret;
}

void EditorPanelProperties::EndPanel() {
    if (m_lastSelection && m_currentSelection != m_lastSelection) {
        CommitEditContext();
    }
    m_lastSelection = m_currentSelection;

    ImGui::PopID();
    EditorPanel::EndPanel();
}

void EditorPanelProperties::DrawContents() {
    if (m_currentSelection) {
        DrawNodeProperties(m_currentSelection);
    }
}

void EditorPanelProperties::DrawNodeProperties(std::shared_ptr<moth_ui::Node> node, bool recurseChildren) {
    auto const entity = node->GetLayoutEntity();

    if (entity->GetType() != moth_ui::LayoutEntityType::Layout) {
        DrawCommonProperties(node);
    }

    switch (entity->GetType()) {
    case moth_ui::LayoutEntityType::Entity:
    case moth_ui::LayoutEntityType::Group:
        // shouldnt get hit
        break;
    case moth_ui::LayoutEntityType::Rect:
        DrawRectProperties(std::static_pointer_cast<moth_ui::NodeRect>(node));
        break;
    case moth_ui::LayoutEntityType::Image:
        DrawImageProperties(std::static_pointer_cast<moth_ui::NodeImage>(node));
        break;
    case moth_ui::LayoutEntityType::Text:
        DrawTextProperties(std::static_pointer_cast<moth_ui::NodeText>(node));
        break;
    case moth_ui::LayoutEntityType::Flipbook:
        DrawFlipbookProperties(std::static_pointer_cast<moth_ui::NodeFlipbook>(node));
        break;
    case moth_ui::LayoutEntityType::Ref:
        DrawRefProperties(std::static_pointer_cast<moth_ui::Group>(node), recurseChildren);
        break;
    case moth_ui::LayoutEntityType::Layout:
        DrawLayoutProperties(std::static_pointer_cast<moth_ui::Group>(node));
        break;
    default:
        break;
    }
}

void EditorPanelProperties::DrawCommonProperties(std::shared_ptr<moth_ui::Node> node) {
    auto const entity = node->GetLayoutEntity();

    ImGui::SeparatorText("Node");

    PropertiesInput<char const*>(
        "ID", entity->m_id.c_str(),
        [&](char const* changedValue) {
            node->SetId(changedValue);
        },
        [this, node, entity](char const* oldValue, char const* newValue) {
            auto action = MakeChangeValueAction(entity->m_id, std::string(oldValue), std::string(newValue), [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<bool>("Visible", node->IsVisible(), [&](bool newValue) {
        auto action = MakeChangeValueAction(entity->m_visible, entity->m_visible, newValue, [node]() { node->ReloadEntity(); });
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput<bool>("Locked", m_editorLayer.IsLocked(node), [&](bool value) {
        auto action = MakeLockAction(node, value, m_editorLayer);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput<bool>("Show Bounds", node->GetShowRect(), [&](bool value) {
        auto action = MakeShowBoundsAction(node, value);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    {
        auto const& sr = node->GetScreenRect();
        int const boundsW = sr.bottomRight.x - sr.topLeft.x;
        int const boundsH = sr.bottomRight.y - sr.topLeft.y;
        float const pivScreenX = static_cast<float>(sr.topLeft.x) + (entity->m_pivot.x * static_cast<float>(boundsW));
        float const pivScreenY = static_cast<float>(sr.topLeft.y) + (entity->m_pivot.y * static_cast<float>(boundsH));

        auto const posText = fmt::format("{:.0f}, {:.0f}", pivScreenX, pivScreenY);
        auto const sizeText = fmt::format("{} x {}", boundsW, boundsH);


        ImGui::Text("Position");
        ImGui::SameLine();
        float const posTextWidth = ImGui::CalcTextSize(posText.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - posTextWidth);
        ImGui::Text("%s", posText.c_str());
        ImGui::Text("Size");
        ImGui::SameLine();
        float const sizTextWidth = ImGui::CalcTextSize(sizeText.c_str()).x;
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - sizTextWidth);
        ImGui::Text("%s", sizeText.c_str());
    }

    // Bounds uses the legacy OnInputFocus path intentionally: InputElement("Bounds", ...)
    // is a composite widget (4×2 InputFloat sub-items) so ImGui::GetItemID() only refers
    // to the last sub-input, making IsItemActivated()/IsItemDeactivatedAfterEdit() unreliable
    // for the whole group. BeginEditBounds is safe to call on consecutive Changed frames —
    // its internal guard prevents duplicate undo sessions — and EndEditBounds is committed
    // when the context is flushed (via IsItemActivated() on the next PropertiesInput, or on
    // selection change in EndPanel).
    auto valueBuffer = GetBufferForValue(node->GetLayoutRect());
    auto const inputContext = InputElement("Bounds", valueBuffer);
    if (inputContext.Changed) {
        m_editorLayer.BeginEditBounds(node);
        node->GetLayoutRect() = *inputContext.ValueBuffer.Buffer;
        node->RecalculateBounds();
    }
    if (inputContext.Focused) {
        OnInputFocus<moth_ui::LayoutRect>("Bounds", node->GetLayoutRect(), [this](moth_ui::LayoutRect, moth_ui::LayoutRect) {
            m_editorLayer.EndEditBounds();
        });
    }

    ImGui::Indent();
    DrawBoundsTools(node);
    ImGui::Unindent();

    PropertiesInput<moth_ui::FloatVec2>(
        "Pivot", entity->m_pivot, {},
        [this, node, entity](moth_ui::FloatVec2 oldValue, moth_ui::FloatVec2 newValue) {
            auto action = MakeChangeValueAction(entity->m_pivot, oldValue, newValue, [node]() {
                node->ReloadEntity();
            });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<float>(
        "Rotation", node->GetRotation(),
        [&](float changedValue) {
            m_editorLayer.BeginEditRotation(node);
            node->SetRotation(changedValue);
        },
        [this](float oldValue, float newValue) {
            m_editorLayer.EndEditRotation();
        });

    PropertiesInput<moth_ui::Color>(
        "Color", node->GetColor(),
        [&](moth_ui::Color changedValue) {
            m_editorLayer.BeginEditColor(node);
            node->SetColor(changedValue);
        },
        [this](moth_ui::Color oldValue, moth_ui::Color newValue) {
            m_editorLayer.EndEditColor();
        });

    PropertiesInput<moth_ui::BlendMode>(
        "Blend Mode", node->GetBlendMode(), {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_blend, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawBoundsTools(std::shared_ptr<moth_ui::Node> node) {
    if (!ImGui::CollapsingHeader("Tools##bounds")) {
        return;
    }


    auto* parent = node->GetParent();
    if (parent == nullptr) {
        ImGui::TextDisabled("(root node has no parent)");
        return;
    }

    auto const& screenRect = node->GetScreenRect();
    auto const& parentRect = parent->GetScreenRect();
    moth_ui::FloatVec2 const parentOffset = static_cast<moth_ui::FloatVec2>(parentRect.topLeft);
    moth_ui::FloatVec2 const parentDim = static_cast<moth_ui::FloatVec2>(parentRect.bottomRight - parentRect.topLeft);
    moth_ui::FloatVec2 const nodeTL = static_cast<moth_ui::FloatVec2>(screenRect.topLeft);
    moth_ui::FloatVec2 const nodeBR = static_cast<moth_ui::FloatVec2>(screenRect.bottomRight);

    // Remap anchor to a new value while keeping screen position constant.
    // From RecalculateBounds: screen = parentOffset + offset + parentDim * anchor
    // So: offset_new = screen - parentOffset - parentDim * anchor_new
    auto applyAnchor = [&](moth_ui::FloatVec2 anchorTL, moth_ui::FloatVec2 anchorBR) {
        m_editorLayer.BeginEditBounds(node);
        auto& lr = node->GetLayoutRect();
        lr.anchor.topLeft = anchorTL;
        lr.anchor.bottomRight = anchorBR;
        lr.offset.topLeft = nodeTL - parentOffset - (parentDim * anchorTL);
        lr.offset.bottomRight = nodeBR - parentOffset - (parentDim * anchorBR);
        node->RecalculateBounds();
        m_editorLayer.EndEditBounds();
    };

    // 9-point anchor preset grid — both corners anchored to the same point
    struct AnchorPreset {
        moth_ui::FloatVec2 anchor;
        char const* label = nullptr;
        char const* tooltip = nullptr;
    };
    static const AnchorPreset presets[3][3] = {
        { { { 0.0f, 0.0f }, "TL", "Top Left" }, { { 0.5f, 0.0f }, "TC", "Top Center" }, { { 1.0f, 0.0f }, "TR", "Top Right" } },
        { { { 0.0f, 0.5f }, "ML", "Middle Left" }, { { 0.5f, 0.5f }, "C", "Center" }, { { 1.0f, 0.5f }, "MR", "Middle Right" } },
        { { { 0.0f, 1.0f }, "BL", "Bottom Left" }, { { 0.5f, 1.0f }, "BC", "Bottom Center" }, { { 1.0f, 1.0f }, "BR", "Bottom Right" } },
    };

    ImGui::Text("Set Anchor:");
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (col > 0) {
                ImGui::SameLine(0, 4);
            }
            auto const& p = presets[row][col];
            ImGui::PushID((row * 3) + col);
            if (ImGui::Button(p.label, ImVec2(24, 24))) {
                applyAnchor(p.anchor, p.anchor);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", p.tooltip);
            }
            ImGui::PopID();
        }
    }

    // Full bounds: anchor TL=(0,0), BR=(1,1) — node stretches with parent
    if (ImGui::Button("Full Bounds")) {
        applyAnchor({ 0.0f, 0.0f }, { 1.0f, 1.0f });
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Anchor TL=(0,0) BR=(1,1); adjust offsets to maintain position");
    }

    // Anchor to offset: zero all offsets; update anchor fractions to maintain position.
    // anchor_new = (screen - parentOffset) / parentDim
    bool const parentValid = (parentDim.x != 0.0f && parentDim.y != 0.0f);
    if (!parentValid) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Anchor to Offset")) {
        m_editorLayer.BeginEditBounds(node);
        auto& lr = node->GetLayoutRect();
        lr.anchor.topLeft = (nodeTL - parentOffset) / parentDim;
        lr.anchor.bottomRight = (nodeBR - parentOffset) / parentDim;
        lr.offset.topLeft = { 0.0f, 0.0f };
        lr.offset.bottomRight = { 0.0f, 0.0f };
        node->RecalculateBounds();
        m_editorLayer.EndEditBounds();
    }
    if (!parentValid) {
        ImGui::EndDisabled();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Zero all offsets; update anchor to maintain current screen position");
    }
}

void EditorPanelProperties::DrawRectProperties(std::shared_ptr<moth_ui::NodeRect> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityRect>(node->GetLayoutEntity());

    ImGui::SeparatorText("Rect");

    PropertiesInput<bool>(
        "Filled", entity->m_filled,
        [&](auto const newValue) {
            auto const oldValue = entity->m_filled;
            auto action = MakeChangeValueAction(entity->m_filled, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawImageProperties(std::shared_ptr<moth_ui::NodeImage> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(node->GetLayoutEntity());

    ImGui::SeparatorText("Image");

    PropertiesInput<moth_ui::TextureFilter>(
        "Texture Filter", entity->m_textureFilter, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_textureFilter, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::ImageScaleType>(
        "Image Scale Type", entity->m_imageScaleType, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_imageScaleType, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<float>(
        "Image Scale", entity->m_imageScale,
        [&](float changedValue) {
            entity->m_imageScale = changedValue;
            node->ReloadEntity();
        },
        [this, node, entity](float oldValue, float newValue) {
            auto action = MakeChangeValueAction(entity->m_imageScale, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::IntRect>(
        "Source Rect", entity->m_sourceRect,
        [&](auto newValue) {
            entity->m_sourceRect = newValue;
            node->ReloadEntity();
        },
        [this, node, entity](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_sourceRect, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::LayoutRect>(
        "Target Borders", entity->m_targetBorders,
        [&](auto newValue) {
            entity->m_targetBorders = newValue;
            node->ReloadEntity();
        },
        [this, node, entity](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_targetBorders, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::IntRect>(
        "Source Borders", entity->m_sourceBorders,
        [&](auto newValue) {
            entity->m_sourceBorders = newValue;
            node->ReloadEntity();
        },
        [this, node, entity](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_sourceBorders, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    auto const& layoutPath = m_editorLayer.GetCurrentLayoutPath();
    auto const imageBase = layoutPath.empty() ? std::filesystem::current_path() : layoutPath.parent_path();
    std::error_code ec;
    auto rel = std::filesystem::relative(entity->m_imagePath, imageBase, ec);
    std::string imagePath = ec ? entity->m_imagePath.string() : rel.string();
    ImGui::InputText("Image Path", imagePath.data(), imagePath.size() + 1, ImGuiInputTextFlags_ReadOnly);

    if (node->GetImage() != nullptr) {
        using namespace moth_ui;

        auto const dims = node->GetImage()->GetDimensions();
        if (dims.x > 0 && dims.y > 0) {
            float const scale = std::min(200.0f / static_cast<float>(dims.x), 200.0f / static_cast<float>(dims.y));
            imgui_ext::Image(node->GetImage(), std::max(1, static_cast<int>(static_cast<float>(dims.x) * scale)), std::max(1, static_cast<int>(static_cast<float>(dims.y) * scale)));

            FloatVec2 const previewImageMin{ ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
            FloatVec2 const previewImageMax{ ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y };
            FloatVec2 const previewImageSize = previewImageMax - previewImageMin;
            FloatVec2 const sourceImageDimensions = static_cast<FloatVec2>(node->GetImage()->GetDimensions());
            FloatVec2 const imageToPreviewFactor = previewImageSize / sourceImageDimensions;
            FloatRect const sourceRect = static_cast<FloatRect>(node->GetSourceRect());

            auto const ImageToPreview = [&](FloatVec2 const& srcVec) -> FloatVec2 {
                return srcVec * imageToPreviewFactor;
            };

            FloatVec2 const srcMin = previewImageMin + ImageToPreview(sourceRect.topLeft);
            FloatVec2 const srcMax = previewImageMin + ImageToPreview(sourceRect.bottomRight);

            auto* drawList = ImGui::GetWindowDrawList();

            // source rect preview
            auto const rectColor = moth_ui::ToABGR(m_editorLayer.GetConfig().PreviewSourceRectColor);
            drawList->AddRect(ImVec2{ srcMin.x, srcMin.y }, ImVec2{ srcMax.x, srcMax.y }, rectColor);

            // 9 slice preview
            auto const sliceColor = moth_ui::ToABGR(m_editorLayer.GetConfig().PreviewImageSliceColor);
            if (node->GetImageScaleType() == moth_ui::ImageScaleType::NineSlice) {
                FloatVec2 const slice1 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(node->GetSourceSlices()[1]));
                FloatVec2 const slice2 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(node->GetSourceSlices()[2]));

                drawList->AddLine(ImVec2{ slice1.x, srcMin.y }, ImVec2{ slice1.x, srcMax.y }, sliceColor);
                drawList->AddLine(ImVec2{ slice2.x, srcMin.y }, ImVec2{ slice2.x, srcMax.y }, sliceColor);
                drawList->AddLine(ImVec2{ srcMin.x, slice1.y }, ImVec2{ srcMax.x, slice1.y }, sliceColor);
                drawList->AddLine(ImVec2{ srcMin.x, slice2.y }, ImVec2{ srcMax.x, slice2.y }, sliceColor);
            }
        } else {
            ImGui::TextUnformatted("(invalid image dimensions)");
        }
    }

    if (ImGui::Button("Load Image..")) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog("jpg,jpeg,png,bmp", currentPath.c_str(), &outPath);

        if (result == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            NFD_Free(outPath);
            auto const targetImageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(node->GetLayoutEntity());
            auto const oldPath = targetImageEntity->m_imagePath;
            auto const newPath = filePath;
            auto action = MakeChangeValueAction(entity->m_imagePath, oldPath, newPath, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        }
    }
}

void EditorPanelProperties::DrawTextProperties(std::shared_ptr<moth_ui::NodeText> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityText>(node->GetLayoutEntity());

    ImGui::SeparatorText("Text");

    PropertiesInput<int>(
        "Font Size", entity->m_fontSize,
        [&](int changedValue) {
            entity->m_fontSize = changedValue;
            node->ReloadEntity();
        },
        [this, node, entity](int oldValue, int newValue) {
            auto action = MakeChangeValueAction(entity->m_fontSize, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    auto const fontNames = m_editorLayer.GetContext().GetFontFactory().GetFontNameList();
    PropertiesInputList(
        "Font", fontNames, entity->m_fontName,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_fontName, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::TextHorizAlignment>(
        "H Alignment", entity->m_horizontalAlignment, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_horizontalAlignment, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::TextVertAlignment>(
        "V Alignment", entity->m_verticalAlignment, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_verticalAlignment, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<bool>(
        "Drop Shadow", entity->m_dropShadow,
        [&](auto newValue) {
            auto const oldValue = entity->m_dropShadow;
            auto action = MakeChangeValueAction(entity->m_dropShadow, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::IntVec2>(
        "Drop Shadow Offset", entity->m_dropShadowOffset,
        [&](auto changedValue) {
            entity->m_dropShadowOffset = changedValue;
            node->ReloadEntity();
        },
        [this, node, entity](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_dropShadowOffset, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::Color>(
        "Drop Shadow Color", entity->m_dropShadowColor,
        [&](auto changedValue) {
            entity->m_dropShadowColor = changedValue;
            node->ReloadEntity();
        },
        [this, node, entity](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_dropShadowColor, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Text", node->GetText().c_str(), 8,
        [&](char const* changedValue) {
            node->SetText(changedValue);
        },
        [this, node, entity](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_text, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawFlipbookProperties(std::shared_ptr<moth_ui::NodeFlipbook> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityFlipbook>(node->GetLayoutEntity());

    ImGui::SeparatorText("Flipbook");

    PropertiesInput<moth_ui::TextureFilter>(
        "Texture Filter", entity->m_textureFilter, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_textureFilter, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    auto const& layoutPath = m_editorLayer.GetCurrentLayoutPath();
    auto const flipbookBase = layoutPath.empty() ? std::filesystem::current_path() : layoutPath.parent_path();
    std::error_code ec;
    auto rel = std::filesystem::relative(entity->m_flipbookPath, flipbookBase, ec);
    std::string displayPath = ec ? entity->m_flipbookPath.string() : rel.string();
    ImGui::InputText("Flipbook Path", displayPath.data(), displayPath.size() + 1, ImGuiInputTextFlags_ReadOnly);

    if (ImGui::Button("Load Flipbook..")) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog("json", currentPath.c_str(), &outPath);

        if (result == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            NFD_Free(outPath);
            auto const oldPath = entity->m_flipbookPath;
            auto action = MakeChangeValueAction(entity->m_flipbookPath, oldPath, filePath, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        }
    }

    // Helper: create or overwrite a discrete keyframe at the current editor frame.
    auto setDiscreteKeyframe = [&](moth_ui::AnimationTrack::Target target, std::string newValue) {
        int const frame = m_editorLayer.GetSelectedFrame();
        auto action = std::make_unique<AddDiscreteKeyframeAction>(entity, target, frame, std::move(newValue));
        m_editorLayer.PerformEditAction(std::move(action));
        m_editorLayer.Refresh();
    };

    // Clip name — dropdown populated from the loaded flipbook.
    {
        auto const clipIt = entity->m_discreteTracks.find(moth_ui::AnimationTrack::Target::FlipbookClip);
        std::string const currentClip = (clipIt != entity->m_discreteTracks.end())
                                            ? clipIt->second.GetValueAtFrame(m_editorLayer.GetSelectedFrame())
                                            : std::string{};
        auto const* flipbook = node->GetFlipbook();
        ImGui::SetNextItemWidth(200.0f);
        if ((flipbook != nullptr) && ImGui::BeginCombo("Clip Name", currentClip.c_str())) {
            for (int i = 0; i < flipbook->GetClipCount(); ++i) {
                auto const clipName = flipbook->GetClipName(i);
                bool selected = (clipName == currentClip);
                if (ImGui::Selectable(std::string(clipName).c_str(), selected)) {
                    setDiscreteKeyframe(moth_ui::AnimationTrack::Target::FlipbookClip, std::string(clipName));
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        } else if (flipbook == nullptr) {
            ImGui::LabelText("Clip Name", "(no flipbook loaded)");
        }
    }

    // Playing — checkbox.
    {
        auto const playIt = entity->m_discreteTracks.find(moth_ui::AnimationTrack::Target::FlipbookPlaying);
        bool playing = (playIt != entity->m_discreteTracks.end()) && (playIt->second.GetValueAtFrame(m_editorLayer.GetSelectedFrame()) == "1");
        if (ImGui::Checkbox("Playing", &playing)) {
            setDiscreteKeyframe(moth_ui::AnimationTrack::Target::FlipbookPlaying, playing ? "1" : "0");
        }
    }

    if (node->GetFlipbook() != nullptr) {
        ImGui::LabelText("Current Clip", "%s", std::string(node->GetCurrentClipName()).c_str());
        ImGui::LabelText("Current Frame", "%d", node->GetCurrentFrame());
    }
}

char const* GetChildName(std::shared_ptr<moth_ui::LayoutEntity> entity) {
    switch (entity->GetType()) {
    case moth_ui::LayoutEntityType::Entity:
        return "Entity";
    case moth_ui::LayoutEntityType::Group:
        return "Group";
        break;
    case moth_ui::LayoutEntityType::Rect:
        return "Rect";
    case moth_ui::LayoutEntityType::Image:
        return "Image";
    case moth_ui::LayoutEntityType::Text:
        return "Text";
    case moth_ui::LayoutEntityType::Flipbook:
        return "Flipbook";
    case moth_ui::LayoutEntityType::Ref:
        return "Ref";
    default:
        return "Unknown";
    }
}

void EditorPanelProperties::DrawRefProperties(std::shared_ptr<moth_ui::Group> node, bool recurseChildren) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityRef>(node->GetLayoutEntity());

    ImGui::SeparatorText("Ref");

    if (recurseChildren) {
        if (ImGui::TreeNode("Children")) {
            int childIndex = 0;
            auto const& children = node->GetChildren();
            for (auto it = std::rbegin(children); it != std::rend(children); ++it) {
                auto const& child = *it;
                static std::string name;
                name = fmt::format("{}: {}", childIndex, GetChildName(child->GetLayoutEntity()));
                if (ImGui::TreeNode(name.c_str())) {
                    DrawNodeProperties(child, false);
                    ImGui::TreePop();
                }
                ++childIndex;
            }
            ImGui::TreePop();
        }
    }
}

void EditorPanelProperties::DrawLayoutProperties(std::shared_ptr<moth_ui::Group> node) {
    auto const entity = std::static_pointer_cast<moth_ui::Layout>(node->GetLayoutEntity());

    PropertiesInput<char const*>(
        "Class", entity->m_class.c_str(),
        [&](std::string changedValue) {
            entity->m_class = changedValue;
        },
        [this, entity](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_class, oldValue, newValue, nullptr);
            m_editorLayer.PerformEditAction(std::move(action));
        });
}
