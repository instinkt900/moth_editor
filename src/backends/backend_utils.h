#pragma once

#include "moth_ui/iimage.h"

std::unique_ptr<moth_ui::IImage> CreateRenderTarget(int width, int height);
std::shared_ptr<moth_ui::IImage> GetRenderTarget();
void SetRenderTarget(std::shared_ptr<moth_ui::IImage> target);

enum class EBlendMode {
	Invalid,
    None,
   	Blend,
    Add,
    Mod,
    Mul,
};

void SetRenderBlendMode(EBlendMode mode);
void SetImageBlendMode(std::shared_ptr<moth_ui::IImage> target, EBlendMode mode);
void SetImageColorMod(std::shared_ptr<moth_ui::IImage> target, moth_ui::Color const& color);
void SetDrawColor(moth_ui::Color const& color);
void RenderClear();
void RenderImage(std::shared_ptr<moth_ui::IImage> image, moth_ui::IntRect const* sourceRect, moth_ui::IntRect const* destRect);
void RenderToPNG(std::filesystem::path const& path);
void RenderRectF(moth_ui::FloatRect const& rect);
void RenderFillRectF(moth_ui::FloatRect const& rect);
void RenderLineF(moth_ui::FloatVec2 const& p0, moth_ui::FloatVec2 const& p1);
void SetRenderLogicalSize(moth_ui::IntVec2 const& logicalSize);
