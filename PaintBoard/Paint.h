#pragma once
#include "Common.h"
#include<DirectXMath.h>

struct PaintColor
{
	float r, g, b, a;
};

class Canvas;

class Paint
{
private:
	bool m_drawing = false;
	POINT m_lastPos{};
	PaintColor m_color{};
	Canvas* m_canvas = nullptr;
	float m_radius = 3.0f;

public:
	void BeginStroke(POINT pos, const PaintColor& color);
	void ContinueStroke(POINT pos);
	void EndStroke();
	
	bool IsDrawing() { return m_drawing; }
	void SetCanvas(Canvas* canvas) { m_canvas = canvas; }
	void SetBrushRadius(float r) { m_radius = r; }


private:
	void PaintDot(POINT pos);
	void PaintLine(POINT from, POINT to);
	
};

