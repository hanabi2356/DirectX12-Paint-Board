#include "PaintUI.h"
#include <windef.h>

PaintUI::PaintUI()
{
	for (int n = 0; n < PaletteSize; ++n)
	{
		ImGui::ColorConvertHSVtoRGB(
			n / static_cast<float>(PaletteSize - 1),
			0.8f, 0.8f,
			m_palette[n].x, m_palette[n].y, m_palette[n].z);
		m_palette[n].w = 1.0f;
	}
}

void PaintUI::Build(Paint& paint, Canvas& canvas)
{
	if (!ImGui::Begin("Paint Tools"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Brush Color");
	ImGui::ColorEdit4("##brush", (float*)&m_brushColor,
		ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

	ImGui::Separator();
	DrawColorPalette();
	DrawCanvas(paint, canvas);
	SaveAndLoadButton(canvas);
	ImGui::End();
}

void PaintUI::DrawColorPalette()
{
	ImGui::Text("Palette");

	const ImGuiColorEditFlags flags =
		ImGuiColorEditFlags_NoAlpha |
		ImGuiColorEditFlags_NoPicker |
		ImGuiColorEditFlags_NoTooltip;

	for (int n = 0; n < PaletteSize; ++n)
	{
		ImGui::PushID(n);

		if ((n % PaletteColumns) != 0)
			ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

		if (ImGui::ColorButton("##swatch", m_palette[n], flags, ImVec2(24, 24)))
		{
			m_brushColor.x = m_palette[n].x;
			m_brushColor.y = m_palette[n].y;
			m_brushColor.z = m_palette[n].z;
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
				memcpy(&m_palette[n], payload->Data, sizeof(float) * 3);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
				memcpy(&m_palette[n], payload->Data, sizeof(float) * 4);
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();
	}
}

void PaintUI::DrawCanvas(Paint& paint, Canvas& canvas)
{
	if (!ImGui::Begin("Canvas"))
	{
		ImGui::End();
		return;
	}
	ImGui::Text("Board | LMB drag: draw");

	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();
	if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
	if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

	if (canvas.GetWidth() != (UINT)canvasSize.x || canvas.GetHeight() != (UINT)canvasSize.y)
	{
		canvas.Resize((UINT)canvasSize.x, (UINT)canvasSize.y);
	}
	ImDrawList* draw = ImGui::GetWindowDrawList();

	//BackGround
	draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(255, 255, 255, 255));
	draw->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(0, 0, 0, 255));

	//입력 받기
	ImGui::InvisibleButton("##canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
	const bool hovered = ImGui::IsItemHovered();
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	POINT pos = {
		(LONG)(mouse.x - canvasPos.x), (LONG)(mouse.y - canvasPos.y)
	};
	ImVec4 c = m_brushColor;
	
	//선 시작
	if (hovered && paint.IsDrawing()==false && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		paint.BeginStroke(pos, { c.x, c.y, c.z, c.w });
	}
	else if (paint.IsDrawing())
	{
		paint.ContinueStroke(pos);
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)==false)
		{
			paint.EndStroke();
		}
	}

	if (paint.IsDrawing())
	{
		paint.ContinueStroke(pos);
		if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			paint.EndStroke();
		}
	}

	//Draw
	const uint8_t* px = canvas.GetPixels();
	const UINT w = canvas.GetWidth();
	const UINT h = canvas.GetHeight();
	if (px && w > 0 && h > 0)
	{
		draw->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);

		const int steps = 4;
		for (UINT y = 0; y < h; y += steps)
		{
			for (UINT x = 0; x < w; x += steps)
			{
				size_t i = (size_t(y) * w + x) * 4;
				if (px[i] > 250 || px[i + 1] < 250 || px[i + 2] < 250)
				{
					ImU32 col = IM_COL32(px[i], px[i + 1], px[i + 2], px[i + 3]);
					draw->AddCircleFilled(ImVec2(canvasPos.x + x, canvasPos.y + y), 1.5f, col, 0);
				}
			}
		}
		draw->PopClipRect();
	}


	ImGui::End();

}

void PaintUI::SaveAndLoadButton(Canvas& canvas)
{
	if (ImGui::Begin("Save&Load") == false)
	{
		ImGui::End();
		return;
	}
	ImGui::Text("Save");

	if (ImGui::Button("Save", ImVec2(50.0f, 20.0f)))
	{
		m_saveLoad.Save(L"paint.png", canvas);
	}
	ImGui::Text("Load");
	if (ImGui::Button("Load", ImVec2(50.0f, 20.0f)))
	{
		m_saveLoad.Load(L"paint.png", canvas);
	}
	ImGui::End();
}
