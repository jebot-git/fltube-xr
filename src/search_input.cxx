// SPDX-License-Identifier: GPL-3.0-or-later
#include "search_input.h"
#include <FL/Fl.H>
#include <algorithm>

int SearchInput::handle(int event) {
    const int key = Fl::event_key();
    const bool enter = key == FL_Enter || key == FL_KP_Enter;
    if (event == FL_UNFOCUS || event == FL_HIDE || event == FL_DEACTIVATE)
        enter_down = false;
    if (event == FL_KEYUP && enter) {
        enter_down = false;
        return 1;
    }
    if (event == FL_KEYDOWN) {
        if (Fl::event_state(FL_CTRL) && key == 'l') {
            take_focus();
            position(size(), 0);
            return 1;
        }
        if (enter) {
            // A callback may dispatch nested focus/key events while opening a
            // player. Latch first, and reset only on release or focus loss.
            if (!enter_down) {
                enter_down = true;
                do_callback();
            }
            return 1;
        }
        // FLTK permits literal Ctrl-M/Ctrl-J text in a single-line Fl_Input.
        // A text-only commit or repeat can carry CR with no Enter keysym.
        // It is never part of a search term or URL; don't let it reach compose.
        const char* text = Fl::event_text();
        if (std::find(text, text + Fl::event_length(), '\r') != text + Fl::event_length()
            || std::find(text, text + Fl::event_length(), '\n') != text + Fl::event_length())
            return 1;
        if (key == FL_Up || key == FL_Down) {
            const auto& history = key == FL_Up ? next_in_history : previous_in_history;
            const std::string value = history ? history() : std::string();
            if (!value.empty()) this->value(value.c_str());
            return 1;
        }
    }
    return Fl_Input::handle(event);
}
