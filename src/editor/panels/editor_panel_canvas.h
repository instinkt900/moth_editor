#pragma once

#include "editor_panel.h"
#include "../editor_layer.h"

class BoundsWidget;

class EditorPanelCanvas : public EditorPanel {
public:
    EditorPanelCanvas(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelCanvas() = default;

    void OnShutdown() override;

    enum class CoordSpace {
        AppSpace,    // pixels from the top left of the whole application
        WindowSpace, // pixels from top left of the imgui canvas window
        WorldSpace,  // units within the scene rendered to the render target containing the canvas
        CanvasSpace, // world space offset to the top left of the canvas
    };

    template <CoordSpace InSpace, CoordSpace OutSpace>
    moth_ui::FloatVec2 ConvertSpace(moth_ui::FloatVec2 const& point) {
        if constexpr (OutSpace == InSpace) {
            return point;
        } else if constexpr (OutSpace == CoordSpace::AppSpace) {
            if constexpr (InSpace == CoordSpace::WindowSpace) {
                // TODO need to take into account borders etc
                auto const windowOffset = static_cast<moth_ui::FloatVec2>(m_canvasWindowPos);
                return point + windowOffset;
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
                // TODO need to take into account borders etc
                auto const windowOffset = static_cast<moth_ui::FloatVec2>(m_canvasWindowPos);
                return point - windowOffset;
            } else if constexpr (InSpace == CoordSpace::WorldSpace) {
                float const scaleFactor = m_canvasZoom / 100.0f;
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
                float const scaleFactor = m_canvasZoom / 100.0f;
                return point / scaleFactor;
            } else if constexpr (InSpace == CoordSpace::CanvasSpace) {
                float const scaleFactor = m_canvasZoom / 100.0f;
                auto const scaledCanvasSize = static_cast<moth_ui::FloatVec2>(m_editorLayer.GetConfig().CanvasSize) * scaleFactor;
                auto const displaySize = static_cast<moth_ui::FloatVec2>(m_canvasWindowSize);
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
                float const scaleFactor = m_canvasZoom / 100.0f;
                auto const scaledCanvasSize = static_cast<moth_ui::FloatVec2>(m_editorLayer.GetConfig().CanvasSize) * scaleFactor;
                auto const displaySize = static_cast<moth_ui::FloatVec2>(m_canvasWindowSize);
                auto const scaledCanvasOffset = m_canvasOffset + (displaySize - scaledCanvasSize) / 2.0f;
                return (point - scaledCanvasOffset) / scaleFactor;
            }
        }
    }

    template <CoordSpace InSpace, CoordSpace OutSpace, typename OutType, typename InType>
    moth_ui::Vector<OutType, 2> ConvertSpace(moth_ui::Vector<InType, 2> const& point) {
        return static_cast<moth_ui::Vector<OutType, 2>>(ConvertSpace<InSpace, OutSpace>(static_cast<moth_ui::FloatVec2>(point)));
    }

    template <CoordSpace InSpace, CoordSpace OutSpace>
    moth_ui::FloatRect ConvertSpace(moth_ui::FloatRect const& rect) {
        moth_ui::FloatRect result;
        result.topLeft = ConvertSpace<InSpace, OutSpace>(rect.topLeft);
        result.bottomRight = ConvertSpace<InSpace, OutSpace>(rect.bottomRight);
        return result;
    }

    template <CoordSpace InSpace, CoordSpace OutSpace, typename OutType, typename InType>
    moth_ui::Rect<OutType> ConvertSpace(moth_ui::Rect<InType> const& rect) {
        return static_cast<moth_ui::Rect<OutType>>(ConvertSpace<InSpace, OutSpace>(static_cast<moth_ui::FloatRect>(rect)));
    }

    EditorLayer& GetEditorLayer() const { return m_editorLayer; }

    moth_ui::IntVec2 SnapToGrid(moth_ui::IntVec2 const& original);

    void ResetView();

private:
    bool BeginPanel() override;
    void DrawContents() override;
    void EndPanel() override;

    moth_ui::IntVec2 m_canvasWindowPos;
    moth_ui::IntVec2 m_canvasWindowSize;

    moth_ui::FloatVec2 m_canvasOffset{ 0, 0 };
    int m_canvasZoom = 100;

    TextureRef m_displayTexture;
    moth_ui::FloatVec2 m_initialCanvasOffset;
    bool m_draggingCanvas = false;
    moth_ui::IntVec2 m_lastMousePos;

    bool m_holdingSelection = false;
    moth_ui::IntVec2 m_grabPosition;

    std::unique_ptr<BoundsWidget> m_boundsWidget;

    bool m_dragSelecting = false;
    moth_ui::IntVec2 m_dragSelectStart;
    moth_ui::IntVec2 m_dragSelectEnd;

    void UpdateDisplayTexture(SDL_Renderer& renderer, moth_ui::IntVec2 const& displaySize);

    void OnMouseClicked(moth_ui::IntVec2 const& appPosition);
    void OnMouseReleased(moth_ui::IntVec2 const& appPosition);
    void OnMouseMoved(moth_ui::IntVec2 const& appPosition);

    void SelectInRect(moth_ui::IntRect const& selectionRect);

    void UpdateInput();

    friend class EditorPanelCanvasProperties;
};
