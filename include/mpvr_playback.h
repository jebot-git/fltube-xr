// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
#include <wordexp.h>
#include <string>
#include <vector>
extern char **environ;

// Keep the webpage URL intact: mpv's yt-dlp hook handles DASH audio, headers,
// seeking and source metadata. Never evaluate shell commands in player options.
inline int fltube_play_mpvr(const std::string& player, const std::string& params,
        const std::string& url, int height, int layout, const std::string& ytdlp) {
    static const char* layouts[] = {"auto", "full-sbs", "half-sbs", "full-tb", "half-tb", "mono"};
    if (layout < 0 || layout > 5) return EINVAL;
    std::vector<std::string> args{player};
    wordexp_t words{};
    int result = wordexp(params.c_str(), &words, WRDE_NOCMD | WRDE_UNDEF);
    if (result) {
        if (result == WRDE_NOSPACE) wordfree(&words);
        return EINVAL;
    }
    for (size_t i = 0; i < words.we_wordc; ++i) args.emplace_back(words.we_wordv[i]);
    wordfree(&words);
    args.push_back("--sbs-layout=" + std::string(layouts[layout]));
    args.push_back("--ytdl-format=bestvideo[height<=?" + std::to_string(height) + "]+bestaudio/best[height<=?" + std::to_string(height) + "]");
    args.push_back("--script-opts-append=ytdl_hook-ytdl_path=" + ytdlp);
    args.push_back("--");
    args.push_back(url);
    std::vector<char*> argv;
    for (auto& arg : args) argv.push_back(arg.data());
    argv.push_back(nullptr);
    pid_t pid;
    result = posix_spawnp(&pid, player.c_str(), nullptr, nullptr, argv.data(), environ);
    if (result) return result;
    int status;
    while (waitpid(pid, &status, 0) == -1) if (errno != EINTR) return errno;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
