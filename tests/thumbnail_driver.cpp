// SPDX-License-Identifier: GPL-3.0-or-later
#include "thumbnail_image.h"
#include "../integration/gpu-image.h"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Gl_Window.H>
#include <cstdio>
#include <cstdlib>
#include <memory>
class ImageWindow : public Fl_Gl_Window {
public:
    ImageWindow() : Fl_Gl_Window(160,120,"FLTube image test") { mode(FL_RGB | FL_DOUBLE); }
    void draw() override {
        glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
        dx::drawing_gl = true;
        Fl_Gl_Window::draw();
        dx::drawing_gl = false;
    }
};
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--render") {
        std::unique_ptr<Fl_Image> image(create_resized_image(argv[2],95,80));
        if (!image) return 2;
        ImageWindow window; window.begin();
        Fl_Box box(20,20,95,80); box.image(image.get());
        window.end(); window.show();
        Fl::add_timeout(8, [](void* p){static_cast<Fl_Window*>(p)->hide();}, &window);
        return Fl::run();
    }
    if (argc != 4) return 99;
    std::unique_ptr<Fl_Image> image(create_resized_image(argv[1], std::atoi(argv[2]), std::atoi(argv[3])));
    if (!image) return 2;
    const int stride = image->ld() ? image->ld() : image->w() * image->d();
    const auto pixel = reinterpret_cast<const unsigned char*>(image->data()[0]) +
        image->h() / 2 * stride + image->w() / 2 * image->d();
    std::printf("{\"w\":%d,\"h\":%d,\"rgb\":[%d,%d,%d]}\n", image->w(), image->h(), pixel[0],pixel[1],pixel[2]);
}
