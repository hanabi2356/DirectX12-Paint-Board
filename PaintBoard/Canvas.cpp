#include "Canvas.h"

void Canvas::Resize(UINT w, UINT h)
{
	if (w == 0 || h == 0) return;

	m_width = w;
	m_height = h;
	m_pixels.assign(static_cast<size_t>(m_width) * m_height * 4, 255);
	m_dirty = true;
}

void Canvas::Clear(float r, float g, float b, float a)
{
	auto ToU8 = [](float v) -> uint8_t {
		if (v < 0.0f) v = 0.0f;
		if (v > 1.0f) v = 1.0f;
		return static_cast<uint8_t>(v * 255.0f + 0.5f);
	};

	const uint8_t R = ToU8(r);
	const uint8_t G = ToU8(g);
	const uint8_t B = ToU8(b);
	const uint8_t A = ToU8(a);

	for (size_t i = 0; i < m_pixels.size(); i += 4)
	{
		m_pixels[i + 0] = R;
		m_pixels[i + 1] = G;
		m_pixels[i + 2] = B;
		m_pixels[i + 3] = A;
	}
	m_dirty = true;
}

void Canvas::DrawDot(int x, int y, float r, float g, float b, float a, float radius)
{
	auto ToU8 = [](float v)->uint8_t {
		if (v < 0) { v = 0; }
		if (v > 1) { v = 1; }
		return (uint8_t)(v * 255.0f + 0.5f);
	};

	const uint8_t R = ToU8(r);
	const uint8_t G = ToU8(g);
	const uint8_t B = ToU8(b);
	const uint8_t A = ToU8(a);

	const int rad = (int)ceilf(radius);
	const float r2 = radius * radius;

	for (int dy = -rad; dy <= rad; ++dy)
	{
		for (int dx = -rad; dx <= rad; ++dx)
		{
			if (dx * dx + dy * dy <= r2)
			{
				PutPixel(x + dx, y + dy, R, G, B, A);
			}
		}
	}
}

void Canvas::PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if (x < 0 || y < 0 || x >= static_cast<int>(m_width) || y >= static_cast<int>(m_height)) return;

	const size_t i = (static_cast<size_t>(y) * m_width + static_cast<size_t>(x)) * 4;
	m_pixels[i + 0] = r;
	m_pixels[i + 1] = g;
	m_pixels[i + 2] = b;
	m_pixels[i + 3] = a;
	m_dirty = true;
}
