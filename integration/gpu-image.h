// SPDX-License-Identifier: MIT
#pragma once
#include <FL/Fl_RGB_Image.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <algorithm>
#include <cstring>
#include <memory>
namespace dx {
inline thread_local bool drawing_gl = false;

// FLTK 1.4's OpenGL graphics driver has no raster-image drawing implementation.
// Keep CPU drawing for image surfaces, and upload pixels for native GL widgets.
class GpuImage : public Fl_RGB_Image {
  static unsigned char* pixels(const Fl_Image& image) {
    auto data = new unsigned char[size_t(image.w()) * image.h() * image.d()];
    const int stride = image.ld() ? image.ld() : image.w() * image.d();
    for (int y = 0; y < image.h(); ++y)
      std::memcpy(data + size_t(y) * image.w() * image.d(), image.data()[0] + y * stride,
                  size_t(image.w()) * image.d());
    return data;
  }
public:
  explicit GpuImage(const Fl_Image& image)
      : Fl_RGB_Image(pixels(image), image.w(), image.h(), image.d()) { alloc_array = 1; }
  Fl_Image* copy(int width, int height) const override {
    std::unique_ptr<Fl_Image> image(Fl_RGB_Image::copy(width, height));
    return image && !image->fail() ? new GpuImage(*image) : nullptr;
  }
  void draw(int x, int y, int width, int height, int cx = 0, int cy = 0) override {
    if (!drawing_gl) { Fl_RGB_Image::draw(x, y, width, height, cx, cy); return; }
    width = std::min(width, w() - cx); height = std::min(height, h() - cy);
    int xx, yy, ww, hh;
    fl_clip_box(x, y, width, height, xx, yy, ww, hh);
    if (ww <= 0 || hh <= 0) return;
    const GLenum formats[] = {GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA};
    if (d() < 1 || d() > 4) return;
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0); glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
#ifdef GL_TEXTURE_RECTANGLE_ARB
    glDisable(GL_TEXTURE_RECTANGLE_ARB);
#endif
    glEnable(GL_TEXTURE_2D); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLuint texture; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); glColor4f(1,1,1,1);
    glTexImage2D(GL_TEXTURE_2D, 0, formats[d()-1], w(), h(), 0, formats[d()-1], GL_UNSIGNED_BYTE, data()[0]);
    const float left = float(cx + xx - x) / w(), top = float(cy + yy - y) / h();
    const float right = left + float(ww) / w(), bottom = top + float(hh) / h();
    glBegin(GL_QUADS);
    glTexCoord2f(left,top); glVertex2i(xx,yy);
    glTexCoord2f(right,top); glVertex2i(xx+ww,yy);
    glTexCoord2f(right,bottom); glVertex2i(xx+ww,yy+hh);
    glTexCoord2f(left,bottom); glVertex2i(xx,yy+hh);
    glEnd();
    // Transient textures avoid retaining names across window/context lifetimes.
    glDeleteTextures(1, &texture);
    glPopClientAttrib(); glPopAttrib();
  }
};
inline Fl_RGB_Image* gpu_image(Fl_Image* owned) {
  std::unique_ptr<Fl_Image> image(owned);
  if (!image || image->fail() || !image->data() || image->w() <= 0 || image->h() <= 0) return nullptr;
  return new GpuImage(*image);
}
} // namespace dx
