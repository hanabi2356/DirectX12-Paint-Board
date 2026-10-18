#define _CRT_SECURE_NO_WARNINGS
#include "SaveLoad.h"
#include<string>
#include<vector>

#define STB_IMAGE_IMPLEMENTATION
#include"../SDK/stb-master/stb-master/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"../SDK/stb-master/stb-master/stb_image_write.h"

static std::string WideToUtf8(const wchar_t* w)
{
    if (!w) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    std::string s(n > 0 ? n - 1 : 0, '\0');
    if (n > 1)
    {
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
    }

    return s;
}
bool SaveLoad::Save(const wchar_t* path, const Canvas& canvas)
{
    if (!path || !canvas.GetPixels() || canvas.GetWidth() == 0 || canvas.GetHeight() == 0)
        return false;
    const std::string utf8 = WideToUtf8(path);
    const int w = static_cast<int>(canvas.GetWidth());
    const int h = static_cast<int>(canvas.GetHeight());
    const int stride = w * 4; // RGBA
    // PNG 저장 (comp=4 → RGBA)
    const int ok = stbi_write_png(utf8.c_str(), w, h, 4, canvas.GetPixels(), stride);
    return ok != 0;
}

bool SaveLoad::Load(const wchar_t* path, Canvas& canvas)
{
    if (!path)
        return false;
    const std::string utf8 = WideToUtf8(path);
    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(utf8.c_str(), &w, &h, &channels, 4); // 강제 RGBA
    if (!data)
        return false;
    canvas.Resize(static_cast<UINT>(w), static_cast<UINT>(h));
    uint8_t* dst = const_cast<uint8_t*>(canvas.GetPixels()); // 또는 Canvas에 non-const GetPixels() 추가
    memcpy(dst, data, static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    // dirty 필요하면 Canvas에 MarkDirty() 추가
    return true;
}
