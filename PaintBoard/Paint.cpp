#include "Paint.h"
#include "Canvas.h"
void Paint::BeginStroke(POINT pos, const PaintColor& color)
{
	m_drawing = true;
	m_color = color;
	m_lastPos = pos;
	PaintDot(pos);
}

void Paint::ContinueStroke(POINT pos)
{
	if (m_drawing == false) return;

	PaintDot(pos);
	m_lastPos = pos;
}

void Paint::EndStroke()
{
	m_drawing = false;
}

void Paint::PaintDot(POINT pos)
{
	if (m_canvas == nullptr) return;
	m_canvas->DrawDot(pos.x, pos.y, m_color.r, m_color.g, m_color.b, m_color.a, m_radius);
}

void Paint::PaintLine(POINT from, POINT to)
{
	int dx = abs(to.x - from.x);
	int dy = abs(to.y - from.y);

	int steps = dx > dy ? dx : dy;
	if (steps == 0) 
	{
		PaintDot(from); return; 
	}
	for (int i = 0; i <= steps; i++)
	{
		POINT p;
		p.x = from.x + (to.x - from.x) * i / steps;
		p.y = from.y + (to.y - from.y) * i / steps;
		PaintDot(p);
	}
}
