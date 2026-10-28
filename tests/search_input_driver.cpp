// SPDX-License-Identifier: GPL-3.0-or-later
#include "search_input.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <cassert>
#include <cstring>
#include <iostream>
static int event(SearchInput& input, int type, int key, const char* text = "", int state = 0) {
    Fl::e_number = type;
    Fl::e_keysym = key;
    Fl::e_text = const_cast<char*>(text);
    Fl::e_length = std::strlen(text);
    Fl::e_state = state;
    return input.handle(type);
}
int main() {
    Fl_Window window(360, 100);
    SearchInput input(10, 10, 340, 30);
    window.end(); window.show(); Fl::check(); input.take_focus();
    input.value("retained search"); input.position(input.size());
    // Literal Ctrl-M / text-only CR can be delivered after player focus changes.
    for (int i = 0; i < 8; ++i) event(input, FL_KEYDOWN, 0, "\r");
    assert(std::string(input.value()) == "retained search");
    int calls = 0;
    input.callback([](Fl_Widget*, void* p) { ++*static_cast<int*>(p); }, &calls);
    event(input, FL_KEYDOWN, FL_Enter, "\r");
    event(input, FL_KEYDOWN, FL_Enter, "\r");
    assert(calls == 1);
    event(input, FL_KEYUP, FL_Enter, "\r");
    event(input, FL_KEYDOWN, FL_Enter, "\r");
    assert(calls == 2);
    event(input, FL_UNFOCUS, 0);
    event(input, FL_FOCUS, 0);
    event(input, FL_KEYDOWN, FL_KP_Enter, "\r");
    assert(calls == 3);
    event(input, FL_KEYUP, FL_KP_Enter, "\r");
    assert(std::string(input.value()) == "retained search");
    event(input, FL_KEYDOWN, 'l', "\f", FL_CTRL);
    assert(input.position() == input.size() && input.mark() == 0);
    input.position(input.size());
    event(input, FL_KEYDOWN, 0, " 日本語 🔒");
    assert(std::string(input.value()) == "retained search 日本語 🔒");
    // Set the latch before callbacks, which can enter a nested event loop.
    struct Reentrant { SearchInput* input; int calls = 0; } nested{&input};
    input.callback([](Fl_Widget*, void* p) {
        auto& test = *static_cast<Reentrant*>(p);
        if (++test.calls == 1) event(*test.input, FL_KEYDOWN, FL_Enter, "\r");
    }, &nested);
    event(input, FL_KEYDOWN, FL_Enter, "\r");
    assert(nested.calls == 1);
    std::cout << "PASS CR rejection, Enter/keypad/repeat/release/focus, reentrant callback, Ctrl+L and Unicode\n";
}
