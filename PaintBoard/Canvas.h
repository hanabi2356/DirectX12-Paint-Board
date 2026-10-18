#pragma once
#include "Common.h"
#include<cstdint>
#include<vector>
class Canvas
{

private:
	UINT m_width = 0, m_height = 0;
	std::vector<uint8_t>m_pixels;
	bool m_dirty = false;


public:
	void Resize(UINT w, UINT h);
	void Clear(float r = 1, float g = 1, float b = 1, float a = 1);
	void DrawDot(int x, int y, float r, float g, float b, float a, float radius = 3.0f);

	UINT GetWidth() const { return m_width; }
	UINT GetHeight() const { return m_height; }
	const uint8_t* GetPixels() const { return m_pixels.data(); }
	bool GetDirtyFlag() const { return m_dirty; }
	void ClearDirty() { m_dirty = false; }

private:
	void PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

};
