#pragma once

#include "editor_panel.h"
#include "../editor_layer.h"
#include "moth/graphics/graphics/image.h"

class BoundsWidget;

class EditorPanelCanvas : public EditorPanel {
public:
    EditorPanelCanvas(EditorLayer& editorLayer, bool visible);
    ~EditorPanelCanvas() override = default;

    void OnShutdown() override;

    enum class CoordSpace {
        AppSpace,    // pixels from the top left of the whole application
        WindowSpace, // pixels from top left of the imgui canvas window
        WorldSpace,  // units within the scene rendered to the render target containing the canvas
        CanvasSpace, // world space offset to the top left of the canvas
    };

    template <CoordSpace InSpace, CoordSpace OutSpace>
    moth::ui::FloatVec2 ConvertSpace(moth::ui::FloatVec2 const& point) {
        if constexpr (OutSpace == InSpace) {
            return point;
        } else if constexpr (OutSpace == CoordSpace::AppSpace) {
            if constexpr (InSpace == CoordSpace::WindowSpace) {
                auto const tl = ImGui::GetWindowContentRegionMin();
                auto const windowOffset = static_cast<moth::ui::FloatVec2>(m_canvasWindowPos);
                return moth::ui::FloatVec2{ tl.x, tl.y } + point + windowOffset;
            } else if constexpr (InSpace == CoordSpace::WorldSpace) {
                auto const windowSpace = ConvertSpace<CoordSpace::WorldSpace, CoordSpace::WindowSpace>(point);
                return ConvertSpace<CoordSpace::WindowSpace, CoordSpace::AppSpace>(windowSpace);
            } else if constexpr (InSpace == CoordSpace::CanvasSpace) {
                auto const worldSpace = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace>(point);
                auto const windowSpace = ConvertSpace<CoordSpace::WorldSpace, CoordSpace::WindowSpace>(worldSpace);
                return ConvertSpace<CoordSpace::WindowSpace, CoordSpace::AppSpace>(windowSpace);
            }
        } else if constexpr (OutSpace == CoordSpace::WindowSpace) {
            if constexpr (InSpace == CoordSpace::AppSpace) {
                auto const tl = ImGui::GetWindowContentRegionMin();
                auto const windowOffset = static_cast<moth::ui::FloatVec2>(m_canvasWindowPos);
                return point - windowOffset - moth::ui::FloatVec2{ tl.x, tl.y };
            } else if constexpr (InSpace == CoordSpace::WorldSpace) {
                float const scaleFactor = static_cast<float>(m_canvasZoom) / 100.0f;
                return point * scaleFactor;
            } else if constexpr (InSpace == CoordSpace::CanvasSpace) {
                auto const worldSpace = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace>(point);
                return ConvertSpace<CoordSpace::WorldSpace, CoordSpace::CanvasSpace>(worldSpace);
            }
        } else if constexpr (OutSpace == CoordSpace::WorldSpace) {
            if constexpr (InSpace == CoordSpace::AppSpace) {
                auto const windowSpace = ConvertSpace<CoordSpace::AppSpace, CoordSpace::WindowSpace>(point);
                return ConvertSpace<CoordSpace::WindowSpace, CoordSpace::WorldSpace>(windowSpace);
            } else if constexpr (InSpace == CoordSpace::WindowSpace) {
                float const scaleFactor = static_cast<float>(m_canvasZoom) / 100.0f;
                return point / scaleFactor;
            } else if constexpr (InSpace == CoordSpace::CanvasSpace) {
                float const scaleFactor = static_cast<float>(m_canvasZoom) / 100.0f;
                auto const scaledCanvasSize = static_cast<moth::ui::FloatVec2>(m_editorLayer.GetConfig().CanvasSize) * scaleFactor;
                auto const displaySize = static_cast<moth::ui::FloatVec2>(m_canvasWindowSize);
                auto const scaledCanvasOffset = m_canvasOffset + (displaySize - scaledCanvasSize) / 2.0f;
                return (point * scaleFactor) + scaledCanvasOffset;
            }
        } else if constexpr (OutSpace == CoordSpace::CanvasSpace) {
            if constexpr (InSpace == CoordSpace::AppSpace) {
                auto const windowSpace = ConvertSpace<CoordSpace::AppSpace, CoordSpace::WindowSpace>(point);
                auto const worldSpace = ConvertSpace<CoordSpace::WindowSpace, CoordSpace::WorldSpace>(windowSpace);
                return ConvertSpace<CoordSpace::WorldSpace, CoordSpace::CanvasSpace>(worldSpace);
            } else if constexpr (InSpace == CoordSpace::WindowSpace) {
                auto const worldSpace = ConvertSpace<CoordSpace::WindowSpace, CoordSpace::WorldSpace>(point);
                return ConvertSpace<CoordSpace::WorldSpace, CoordSpace::CanvasSpace>(worldSpace);
            } else if constexpr (InSpace == CoordSpace::WorldSpace) {
                float const scaleFactor = static_cast<float>(m_canvasZoom) / 100.0f;
                auto const scaledCanvasSize = static_cast<moth::ui::FloatVec2>(m_editorLayer.GetConfig().CanvasSize) * scaleFactor;
                auto const displaySize = static_cast<moth::ui::FloatVec2>(m_canvasWindowSize);
                auto const scaledCanvasOffset = m_canvasOffset + (displaySize - scaledCanvasSize) / 2.0f;
                return (point - scaledCanvasOffset) / scaleFactor;
            }
        }
    }

    template <CoordSpace InSpace, CoordSpace OutSpace, typename OutType, typename InType>
    moth::ui::Vector<OutType, 2> ConvertSpace(moth::ui::Vector<InType, 2> const& point) {
        return static_cast<moth::ui::Vector<OutType, 2>>(ConvertSpace<InSpace, OutSpace>(static_cast<moth::ui::FloatVec2>(point)));
    }

    template <CoordSpace InSpace, CoordSpace OutSpace>
    moth::ui::FloatRect ConvertSpace(moth::ui::FloatRect const& rect) {
        moth::ui::FloatRect result;
        result.topLeft = ConvertSpace<InSpace, OutSpace>(rect.topLeft);
        result.bottomRight = ConvertSpace<InSpace, OutSpace>(rect.bottomRight);
        return result;
    }

    template <CoordSpace InSpace, CoordSpace OutSpace, typename OutType, typename InType>
    moth::ui::Rect<OutType> ConvertSpace(moth::ui::Rect<InType> const& rect) {
        return static_cast<moth::ui::Rect<OutType>>(ConvertSpace<InSpace, OutSpace>(static_cast<moth::ui::FloatRect>(rect)));
    }

    EditorLayer& GetEditorLayer() const { return m_editorLayer; }

    moth::ui::IntVec2 SnapToGrid(moth::ui::IntVec2 const& original);

    void ResetView();

private:
    bool BeginPanel() override;
    void DrawContents() override;
    void EndPanel() override;

    moth::ui::IntVec2 m_canvasWindowPos;
    moth::ui::IntVec2 m_canvasWindowSize;

    moth::ui::FloatVec2 m_canvasOffset{ 0, 0 };
    int m_canvasZoom = 100;

    std::shared_ptr<moth::gfx::ITarget> m_displayTexture;
    moth::gfx::Image m_referenceImage;
    std::string m_referenceImagePath;
    moth::ui::FloatVec2 m_initialCanvasOffset;
    bool m_draggingCanvas = false;
    moth::ui::IntVec2 m_lastMousePos;

    bool m_holdingSelection = false;
    moth::ui::IntVec2 m_grabPosition;

    std::unique_ptr<BoundsWidget> m_boundsWidget;

    bool m_dragSelecting = false;
    moth::ui::IntVec2 m_dragSelectStart;
    moth::ui::IntVec2 m_dragSelectEnd;

    void UpdateDisplayTexture(moth::ui::IntVec2 const& displaySize);

    void BeginSelectionGrab(moth::ui::IntVec2 const& worldPosition);
    void EndSelectionGrab();

    void OnMouseClicked(moth::ui::IntVec2 const& appPosition);
    void OnMouseReleased(moth::ui::IntVec2 const& appPosition);
    void OnMouseMoved(moth::ui::IntVec2 const& appPosition);

    std::shared_ptr<moth::ui::Node> GetAtPoint(moth::ui::IntVec2 const& selectionPoint);
    void SelectInRect(moth::ui::IntRect const& selectionRect);

    void UpdateInput();

};
