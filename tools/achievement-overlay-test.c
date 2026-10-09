#include "achievements.h"
#include "achievement_overlay.h"
#include "common_rtl.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
uint8 g_ram[0x20000];
static int sound_calls;
void sciv_achievement_sound_play(void) { sound_calls++; }
int sciv_boss_rush_status(void) { return 0; }
int main(void) {
    assert(sciv_achievements_set_enabled(1)); assert(sciv_achievements_reset());
    g_ram[0x32]=4; g_ram[0x70]=5; g_ram[0x13f4]=16; g_ram[0x13f2]=0x99;
    sciv_achievements_tick(); assert(sciv_achievements_toast()==1);
    int w=768,h=672; uint32_t *pixels=calloc(w*h,4);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) pixels[y*w+x]=0xff182430 | ((x/8)<<16);
    sciv_achievement_overlay((uint8_t *)pixels,w*4,w,h);
    assert(sound_calls==1);
    sciv_achievement_overlay((uint8_t *)pixels,w*4,w,h);
    assert(sound_calls==1); /* Sound is per notification, not per frame. */
    FILE *f=fopen("achievement-preview.ppm","wb"); assert(f); fprintf(f,"P6\n%d %d\n255\n",w,h);
    for(int i=0;i<w*h;i++) { fputc(pixels[i]>>16,f); fputc(pixels[i]>>8,f); fputc(pixels[i],f); }
    fclose(f); free(pixels); return 0;
}
