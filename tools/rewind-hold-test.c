/* Run the shared ring regressions, then exercise this title's hold mode. */
#include "common_cpu_infra.h"
const RtlGameInfo *g_rtl_game_info;
#include <stdlib.h>
#ifdef _WIN32
#define setenv(name, value, overwrite) _putenv_s(name, value)
#endif
#define RtlLoadSnapshotFromMemory test_load_snapshot
#define main shared_rewind_test
#include "../snesrecomp/tests/rewind/rewind_test.c"
#undef main
#undef RtlLoadSnapshotFromMemory
#include "common_rtl.h"
bool RtlLoadSnapshotFromMemory(const void *data, size_t size) {
    return test_load_snapshot(data, size) != 0;
}

int main(void) {
    if (shared_rewind_test()) return 1;
    uint32_t image[256 * 224], output[256 * 224];
    snes_rewind_set_hold_mode(1);
    snes_rewind_configure();
    g_fake_frame = 0;
    for (int frame = 1; frame <= 5; ++frame) {
        for (int i = 0; i < 256 * 224; ++i) image[i] = frame == 5 ? 0xffff0000u : 0xff00ff00u;
        g_fake_frame = frame;
        snes_rewind_note_framebuffer(image, 256, 224);
        snes_rewind_note_frame();
    }
    check(snes_rewind_open(), "hold rewind opens with history");
    g_refuse_load = 1;
    check(!snes_rewind_pop(output, 256, 224) && g_fake_frame == 5,
          "failed hold restore preserves the machine and history");
    g_refuse_load = 0;
    check(snes_rewind_pop(output, 256, 224) && g_fake_frame == 5 && output[0] == 0xffff0000u,
          "hold restores the matching machine and picture");
    check(snes_rewind_pop(output, 256, 224) && g_fake_frame == 4 && output[0] == 0xff00ff00u,
          "holding moves backwards through snapshots");
    snes_rewind_close();
    check(!snes_rewind_is_open() && g_fake_frame == 4, "release retains the restored state");
    advance(2);
    check(snes_rewind_open(), "history captures again after resuming");
    int pops = 0;
    while (snes_rewind_pop(output, 256, 224)) ++pops;
    check(pops > 0 && g_fake_frame == 1, "oldest history stops without wrapping");
    snes_rewind_close();
    snes_rewind_shutdown();
    static uint32_t wide[800 * 224], restored[800 * 224];
    snes_rewind_configure();
    for (int frame=1; frame<=5; ++frame) {
        for (int i=0; i<800*224; ++i) wide[i]=0xff00ff00u;
        wide[799]=0xffff0000u;
        g_fake_frame=frame;
        snes_rewind_note_framebuffer(wide,800,224);
        snes_rewind_note_frame();
    }
    check(snes_rewind_open(), "ultrawide hold opens");
    check(snes_rewind_pop(restored,800,224) && restored[799]==0xffff0000u,
          "ultrawide restores the rightmost pixel and machine");
    check(snes_rewind_pop(restored,800,224) && g_fake_frame==4,
          "ultrawide holding travels backwards");
    check(snes_rewind_pop(restored,256,224) && g_fake_frame==3 && restored[0]==0xff00ff00u,
          "changing view width does not disable rewind");
    snes_rewind_close();
    snes_rewind_shutdown();
    return failures;
}
