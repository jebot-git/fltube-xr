// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <FL/Fl_Input.H>
#include <functional>
#include <memory>
#include <string>
class YtDlp_Helper;
class SearchInput : public Fl_Input {
    bool enter_down = false;
    std::function<std::string()> next_in_history, previous_in_history;
public:
    SearchInput(int x, int y, int w, int h, const char* label = nullptr)
        : Fl_Input(x, y, w, h, label) {}
    void set_search_source(std::shared_ptr<YtDlp_Helper> source);
    int handle(int event) override;
};
