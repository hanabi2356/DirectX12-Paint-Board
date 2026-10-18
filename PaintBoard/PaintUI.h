#pragma once

#include "../SDK/imgui-master/imgui.h"
#include"Canvas.h"
#include"Paint.h"
#include "SaveLoad.h"
class PaintUI
{
public:
	static constexpr int PaletteSize = 24;
	static constexpr int PaletteColumns = 8;

	PaintUI();

	void Build(Paint& paint, Canvas& canvas); // call after ImGui::NewFrame()

	ImVec4 GetBrushColor() const { return m_brushColor; }
	void SetBrushColor(const ImVec4& color) { m_brushColor = color; }

private:
	void DrawColorPalette();
	void DrawCanvas(Paint& paint, Canvas& canvas);
	void SaveAndLoadButton(Canvas& canvas);
private:
	ImVec4 m_brushColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImVec4 m_palette[PaletteSize]{};
	ImVector<ImVec2> m_points;
	
	SaveLoad m_saveLoad;
};
