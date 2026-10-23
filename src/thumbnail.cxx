// SPDX-License-Identifier: GPL-3.0-or-later
#include "thumbnail_image.h"
#include "../integration/gpu-image.h"
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_RGB_Image.H>
#include <webp/decode.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <vector>

Fl_Image* create_resized_image(const std::string& path, int width, int height) {
    if (width <= 0 || height < 0) return nullptr;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto length = file.tellg();
    if (!file || length <= 0 || length > 16 * 1024 * 1024) return nullptr;
    file.seekg(0);
    std::vector<unsigned char> bytes(static_cast<size_t>(length));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) return nullptr;
    auto fit = [&](Fl_Image* image) -> Fl_Image* {
        if (!image || image->fail() || image->w() <= 0 || image->h() <= 0) return nullptr;
        const double scale = std::min(double(width) / image->w(),
            height ? double(height) / image->h() : double(width) / image->w());
        return dx::gpu_image(image->copy(std::max(1, int(std::floor(image->w() * scale))),
                           std::max(1, int(std::floor(image->h() * scale)))));
    };
    if (bytes.size() >= 12 && !std::memcmp(bytes.data(), "RIFF", 4) &&
        !std::memcmp(bytes.data() + 8, "WEBP", 4)) {
        int w = 0, h = 0;
        if (!WebPGetInfo(bytes.data(), bytes.size(), &w, &h) || w <= 0 || h <= 0 ||
            static_cast<uint64_t>(w) * h > 16 * 1024 * 1024) return nullptr;
        auto pixels = std::make_unique<unsigned char[]>(size_t(w) * h * 4);
        if (!WebPDecodeRGBAInto(bytes.data(), bytes.size(), pixels.get(), size_t(w) * h * 4, w * 4))
            return nullptr;
        Fl_RGB_Image image(pixels.get(), w, h, 4);
        return fit(&image);
    }
    // FLTK detects JPEG/PNG/GIF by their contents, irrespective of file suffix.
    fl_register_images();
    auto image = Fl_Shared_Image::get(path.c_str());
    auto result = fit(image);
    if (image) image->release();
    return result;
}
