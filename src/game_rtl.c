/*
 * Super Castlevania IV — per-game runtime glue.
 *
 * THIS FILE IS THE PORT. Everything else the scaffold produced is layout,
 * build wiring, and packaging; this is where the actual work happens.
 *
 * The framework does not, and cannot, drive an arbitrary SNES game on its
 * own. Two responsibilities always land on the game host:
 *
 *   1. Deciding what "one frame" means for THIS title, and returning from
 *      run_frame() at that boundary. A real ROM's reset vector never
 *      returns — it enters a main loop that waits on vblank — so somebody
 *      has to choose the yield point.
 *   2. Delivering NMI/IRQ at the hardware edge (see
 *      snesrecomp/docs/FRAME_MODEL_HOSTS.md).
 *
 * A new port starts on the framework's beam-aligned driver
 * (runner/src/beam_frame_driver.h): one frame is one PPU field, cut at the
 * beam's vblank edge; interrupts -- raster splits and a coprocessor's IRQ
 * line -- are taken where the beam latches them; parked time still runs the
 * beam, APU and coprocessors; and the field is rasterized from the raster
 * journal with real HDMA. It is the framework's, so a fix to it reaches this
 * port on a submodule pull.
 *
 * This file used to hold its own copy of a simpler driver: frames measured
 * from wherever the CPU last stopped, NMI delivered at an arbitrary beam
 * position, the field drawn with end-of-frame register state. A game that
 * uploads in vblank between INIDISP=$80 and $0F flickered black, and one
 * that chains raster splits fell apart. Replace these calls only for what
 * the driver does not do; MetalWarriorsSNESRecomp's src/mw_rtl.c and
 * GundamWingEndlessDuelSNESRecomp's src/game_rtl.c are developed examples of
 * title-specific drivers.
 */

#include "game_rtl.h"
#include "boss_rush.h"
#include "level_select.h"
#include "achievements.h"
#include "other_castle.h"
#include "title_menu.h"
#include "sciv_widescreen.h"

#include "beam_frame_driver.h"
#include "common_rtl.h"
#include "snes/saveload.h"
#include <stdlib.h>
#include <stdio.h>

static void GameSaveExecution(SaveLoadInfo *sli) {
    RtlSaveExecutionState(sli);
    snes_beam_frame_driver_save_state(sli);
}

static void GameLoadExecution(SaveLoadInfo *sli, uint32_t version) {
    (void)version;
    if (!RtlLoadExecutionState(sli)) {
        RtlStateStreamFail(sli);
        return;
    }
    snes_beam_frame_driver_load_state(sli);
}

static void GameExecutionLoaded(uint32_t version) {
    sciv_title_reset();
    (void)version;
    RtlApplyExecutionState();
    if(sciv_other_castle_active()) return;
    sciv_boss_rush_restore();
    sciv_achievements_timeline_changed();
}

void GameRunOneFrame(void)
{
    /* USA NMI $81D4 checks $003C: nonzero means the previous main-loop
     * update is still busy. Opt-in measurement for slowdown comparisons. */
    static unsigned timing_frames, timing_busy;
    static int timing_report = -1;
    if (timing_report < 0) timing_report = getenv("SCIV_TIMING_REPORT") != NULL;
    if (timing_report && g_ram[0x70] == 5) {
        timing_frames++;
        if (g_ram[0x3c] || g_ram[0x3d]) timing_busy++;
        if (timing_frames % 600 == 0)
            fprintf(stderr, "[timing] gameplay fields=%u busy-at-vblank=%u\n",
                    timing_frames, timing_busy);
    }
    snes_beam_frame_driver_run_frame();
    if(sciv_other_castle_active()) return;
    sciv_achievements_tick();
    sciv_boss_rush_tick();
    sciv_one_life_tick();
    sciv_level_select_tick();
    sciv_achievements_toast_elapsed(17);
}

void GameDrawPpuFrame(void)
{
    sciv_widescreen_draw_policy();
    snes_beam_frame_driver_draw_ppu_frame();
}

void GameSessionReset(void)
{
    sciv_title_reset();
    /* Rematch / soft-return: clear anything that must not survive a new
     * session. recomp-ai-rules/NETPLAY.md §3 — sticky state that "has always
     * been fine" is the usual desync culprit, because single-player never
     * re-enters the boot path twice in one process. */
    snes_beam_frame_driver_reset();
    sciv_achievements_timeline_changed();
}

static bool one_life_reset_pending;
void sciv_one_life_reset_machine(void) {
    /* Called inside RtlRunFrame: do not reset its live CPU/APU clocks. */
    one_life_reset_pending = true;
}
void sciv_one_life_before_frame(void) {
    if (!one_life_reset_pending) return;
    one_life_reset_pending = false;
    /* The desktop calls this outside RtlRunFrame, before audio production. */
    RtlReset(1);
    GameSessionReset();
}

static bool GameSnapshotAllowed(void) { return !sciv_one_life_active(); }

const RtlGameInfo kGameInfo = {
    .snapshot_allowed = &GameSnapshotAllowed,
    .title = "supercastlevaniaiv",
    .initialize = NULL,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
    .session_reset = &GameSessionReset,
    .state_save_extra = &GameSaveExecution,
    .state_load_extra = &GameLoadExecution,
    .on_state_loaded = &GameExecutionLoaded,
};
