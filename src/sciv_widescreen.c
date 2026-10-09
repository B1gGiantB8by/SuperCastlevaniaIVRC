/* Presentation policy inspired by MegaManXSNESRecomp's MmxPrepareFrame and
 * MmxRendererViewport. Castlevania uses the shared widened PPU instead of
 * MMX's title-specific room decompressor/compositor. No guest state changes.
 */
#include "sciv_widescreen.h"
#include "sciv_widescreen_tiles.h"
#include "bezel.h"
#include "desktop/config.h"
#include "desktop/host_main.h"
#include "common_rtl.h"
#include "snes/ppu.h"
#include <stdio.h>

static int enabled, aspect = 1, override = -1;
static void save(void) {
    FILE *f = fopen("widescreen.cfg", "w");
    if (f) { fprintf(f, "%d %d\n", enabled, aspect); fclose(f); }
    else fprintf(stderr, "[widescreen] Could not save widescreen.cfg\n");
}
void sciv_widescreen_init(void) {
    enabled = 0; aspect = 1;
    int saved_enabled = 0;
    FILE *f = fopen("widescreen.cfg", "r");
    if (f) { if (fscanf(f, "%d %d", &saved_enabled, &aspect) != 2) { saved_enabled=0; aspect=1; } fclose(f); }
    if (aspect < 0 || aspect > 3) aspect = 1;
    /* Shelved: retain the saved preference for future development, but do
     * not activate a hidden option on ordinary launches. CLI testing remains. */
    enabled = 0;
    if (override >= 0) enabled = override;
}
int sciv_widescreen_enabled(void) { return enabled; }
void sciv_widescreen_set(int value) {
    if (value && sciv_bezel_enabled()) sciv_bezel_set(0);
    enabled = !!value;
    save();
}
int sciv_widescreen_aspect(void) { return aspect; }
void sciv_widescreen_set_aspect(int value) {
    aspect = value >= 0 && value <= 3 ? value : 1;
    save();
}
void sciv_widescreen_override(int value) { override = !!value; }
void sciv_widescreen_prepare(int dw, int dh, int *width, int *height) {
    *height = 224; *width = 256;
    if (!enabled) return;
    static const double ratios[] = {0, 16.0/9.0, 21.0/9.0, 32.0/9.0};
    double target = aspect ? ratios[aspect] : (dh > 0 && dw > 0 ? (double)dw/dh : 16.0/9.0);
    SnesDisplayFrame view = SnesDisplayAspect_ComputeAdaptiveFrame(
        256, 224, kPpuBufWidth, target, SnesDisplayAspect_Clamp(g_config.display_aspect));
    int extra = PpuWsExtraOverride();
    *width = extra >= 0 ? 256 + 2*extra : view.width;
}
void sciv_widescreen_draw_policy(void) {
    int extra = (snesrecomp_desktop_frame_width() - 256) / 2;
    /* Gameplay state $0005 is established by the USA stage driver. Menus,
     * maps and cutscenes retain their centered authentic image. */
    int gameplay = enabled && g_ram[0x70] == 5 && g_ram[0x71] == 0;
    int mapped = sciv_widescreen_tiles_begin(g_ppu);
    gameplay = gameplay && mapped;
    static unsigned diagnostic_frames;
    if (getenv("SCIV_WIDESCREEN_DIAGNOSTICS") && g_ram[0x70]==5 && diagnostic_frames++<4)
        fprintf(stderr,"[wide-policy] enabled=%d mapped=%d gameplay=%d extra=%d mode=%d\n",enabled,mapped,gameplay,extra,PPU_mode(g_ppu));
    if (!gameplay) {
        PpuSetWidescreenTileResolver(g_ppu, NULL, NULL);
        g_ppu->widescreenPresentationLine=NULL;
    }
    PpuSetExtraSpaceCentered(g_ppu, extra);
    if (gameplay) PpuSetExtraSideSpace(g_ppu, extra, extra, 0);
    /* The status strip is 32 scanlines, above the world's scrolling field.
     * BG3 has no verified room source and stays native-width. */
    for (int layer=0; layer<4; ++layer)
        PpuSetWidescreenLayerClampBand(g_ppu, layer, 0, gameplay ? 32 : 0);
    PpuSetWidescreenBg3Widen(g_ppu, 0);
    PpuSetWsHudOamShift(g_ppu, 0);
    /* Nine-bit negative native OAM coordinates must not turn into phantom
     * right-margin sprites. Castlevania has no expanded object staging. */
    static const uint8_t no_right_sprites[16] = {0};
    PpuWsSetOamRightHints(g_ppu, extra ? no_right_sprites : NULL);
}

int sciv_widescreen_view_offset(int extra, int camera, int first, int last) {
    if (extra<=0 || first>last || camera<first || camera>last) return extra;
    int origin=camera-extra, end=last-2*extra;
    if(end<first) end=first;
    if(origin>end) origin=end;
    if(origin<first) origin=first;
    return camera-origin;
}
