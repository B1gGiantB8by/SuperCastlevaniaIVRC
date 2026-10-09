#include "sciv_widescreen.h"
#include "sciv_widescreen_tiles.h"
#include "bezel.h"
#include "desktop/config.h"
#include "common_rtl.h"
#include "snes/ppu.h"
#include <assert.h>
#include <stdio.h>
Config g_config;
/* Rendering is exercised by the paired game runs. These inert host stubs
 * allow the settings/geometry tests to link without a ROM or SDL window. */
Ppu *g_ppu;
int sciv_widescreen_tiles_begin(Ppu *p) { (void)p; return 0; }
void PpuSetWidescreenTileResolver(Ppu *p, PpuWidescreenTileResolver *r, void *c) { (void)p; (void)r; (void)c; }
uint8 g_ram[0x20000];
int snesrecomp_desktop_frame_width(void) { return 256; }
void PpuSetExtraSpaceCentered(Ppu *p, uint16_t n) { (void)p; (void)n; }
void PpuSetExtraSideSpace(Ppu *p, int l, int r, int b) { (void)p; (void)l; (void)r; (void)b; }
void PpuSetWidescreenLayerClampBand(Ppu *p, uint8_t l, uint8_t a, uint8_t b) { (void)p; (void)l; (void)a; (void)b; }
void PpuSetWidescreenBg3Widen(Ppu *p, uint8_t y) { (void)p; (void)y; }
void PpuSetWsHudOamShift(Ppu *p, uint8_t n) { (void)p; (void)n; }
void PpuWsSetOamRightHints(Ppu *p, const uint8_t *h) { (void)p; (void)h; }
int PpuWsExtraOverride(void) { return -1; }
static void write_config(const char *path, const char *value) {
    FILE *f=fopen(path,"w"); assert(f); fputs(value,f); fclose(f);
}
static void reload(void) { sciv_bezel_init(); sciv_widescreen_init(); }
int main(void) {
    write_config("bezel.cfg","0\n"); write_config("widescreen.cfg","0 1\n"); reload();
    assert(!sciv_bezel_enabled() && !sciv_widescreen_enabled());
    sciv_bezel_set(1); sciv_widescreen_set(1);
    assert(!sciv_bezel_enabled() && sciv_widescreen_enabled());
    reload(); assert(!sciv_bezel_enabled() && !sciv_widescreen_enabled());
    sciv_bezel_set(1);
    assert(sciv_bezel_enabled() && !sciv_widescreen_enabled());
    reload(); assert(sciv_bezel_enabled() && !sciv_widescreen_enabled());
    write_config("widescreen.cfg","1 3\n"); reload();
    assert(sciv_bezel_enabled() && !sciv_widescreen_enabled());
    write_config("bezel.cfg","0\n"); write_config("widescreen.cfg","broken\n"); reload();
    assert(!sciv_widescreen_enabled() && sciv_widescreen_aspect()==1);
    sciv_widescreen_set(1);
    int w,h;
    sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==342 && h==224);
    sciv_widescreen_set_aspect(2); sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==448);
    sciv_widescreen_set_aspect(3); sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==682);
    sciv_widescreen_set_aspect(0);
    sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==342);
    sciv_widescreen_prepare(800,1200,&w,&h); assert(w==256);
    sciv_widescreen_prepare(10000,100,&w,&h); assert(w==800);
    sciv_widescreen_prepare(0,0,&w,&h); assert(w==342);
    g_config.display_aspect=1; sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==398);
    g_config.display_aspect=2; sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==456);
    sciv_widescreen_set(0); sciv_widescreen_prepare(1920,1080,&w,&h); assert(w==256);
    write_config("bezel.cfg","1\n"); sciv_widescreen_override(1); reload();
    assert(!sciv_bezel_enabled() && sciv_widescreen_enabled());
    sciv_widescreen_override(0); reload(); assert(sciv_bezel_enabled() && !sciv_widescreen_enabled());
    assert(sciv_widescreen_view_offset(43,0,0,768)==0);
    assert(sciv_widescreen_view_offset(43,20,0,768)==20);
    assert(sciv_widescreen_view_offset(43,400,0,768)==43);
    assert(sciv_widescreen_view_offset(43,748,0,768)==66);
    assert(sciv_widescreen_view_offset(43,768,0,768)==86);
    assert(sciv_widescreen_view_offset(272,768,0,768)==544);
    assert(sciv_widescreen_view_offset(43,1024,1024,1792)==0);
    assert(sciv_widescreen_view_offset(43,1792,1024,1792)==86);
    assert(sciv_widescreen_view_offset(43,100,200,300)==43);
    puts("Widescreen persistence, Bezel exclusion, overrides and geometry passed");
    return 0;
}
