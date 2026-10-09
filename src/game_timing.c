#include "game_timing.h"
#include "cpu_state.h"
#include <stdio.h>

static int override = -1;
int sciv_timing_enhanced(void) { return snes_cpu_timing_divisor == 2; }
void sciv_timing_override(int enhanced) { override = !!enhanced; }
void sciv_timing_init(void) {
    int enhanced = 0;
    FILE *f = fopen("game-timing.cfg", "r");
    if (f) { if (fscanf(f, "%d", &enhanced) != 1) enhanced = 0; fclose(f); }
    snes_cpu_timing_divisor = (override >= 0 ? override : enhanced == 1) ? 2 : 1;
}
void sciv_timing_set(int enhanced) {
    snes_cpu_timing_divisor = enhanced ? 2 : 1;
    FILE *f = fopen("game-timing.cfg", "w");
    if (f) { fprintf(f, "%d\n", !!enhanced); fclose(f); }
    else fprintf(stderr, "[timing] Could not save game-timing.cfg\n");
}
