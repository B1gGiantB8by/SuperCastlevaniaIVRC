#pragma once
#include "snes/ppu.h"
int sciv_widescreen_tiles_begin(Ppu *ppu);
/* Read-only room decoder; exposed for comparison against the native DMA map. */
int sciv_widescreen_room_tile(const uint8_t *ram, int layer, int pages_w,
                             int pages_h, int x, int y, uint16_t *entry);

int sciv_widescreen_view_offset(int extra, int camera, int first, int last);
