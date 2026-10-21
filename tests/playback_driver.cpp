#include "mpvr_playback.h"
int main(int argc, char** argv) {
    if (argc != 5) return 99;
    return fltube_play_mpvr(argv[1], argv[2], argv[3], 1080, 4, argv[4]);
}
