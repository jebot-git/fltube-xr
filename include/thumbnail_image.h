// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <FL/Fl_Image.H>
#include <string>

// Returns an owned image fitted inside the requested box, or nullptr on failure.
Fl_Image* create_resized_image(const std::string& path, int width, int height = 0);
