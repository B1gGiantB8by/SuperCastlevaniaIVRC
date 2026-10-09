/* SCIV USA's expanded room map, as documented by bogaa/SC4Ed:
 * LoadLayout, LoadLevelLayout, and RenderED::RefreshMapCache. These are guest
 * decompressed maps, not a copy of the rolling PPU screen. No ROM/WRAM writes.
 */
#include "sciv_widescreen_tiles.h"
#include "common_rtl.h"
#include "common_cpu_infra.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int pages_w, pages_h, section_first, section_last;
typedef struct Row {
    unsigned y, scroll, map, chars;
    int checked, valid, periodic;
} Row;
static Row rows[2];
static unsigned word(const uint8_t *r, unsigned a) { return r[a] | r[a+1]<<8; }

int sciv_widescreen_room_tile(const uint8_t *ram, int layer, int pw, int ph,
                             int x, int y, uint16_t *entry) {
    if (!ram || !entry || layer<0 || layer>1 || pw<=0 || ph<=0 || pw*ph>64 ||
        x<0 || y<0 || x>=pw*256 || y>=ph*256) return 0;
    unsigned scene = layer ? 0xe000 : 0x8000;
    unsigned blocks = layer ? 0xc000 : 0x6000;
    unsigned index = (y/256*pw + x/256)*64 + (y%256/32)*8 + x%256/32;
    unsigned block = word(ram, scene+index*2);
    int tx = x%32/8, ty = y%32/8;
    if (block&0x8000) tx=3-tx;
    if (block&0x4000) ty=3-ty;
    *entry = word(ram, blocks+(block&255)*32+ty*8+tx*2) ^
        ((block&0x8000)>>1) ^ ((block&0x4000)<<1);
    return 1;
}
/* Clamp the visible world to the guest's current section camera limits.
 * Like SMW's SmwViewOffset, this changes presentation, not the guest camera.
 * Use the raster camera (RAM can already be one simulation tick ahead). */
static uint16_t original_scroll[4];
static int line_shift, line_camera, remapped;
static int camera_x(unsigned scroll) {
    int anchor=word(g_ram,0x1c);
    int x=(anchor&~1023)+(scroll&1023);
    if(x-anchor>512) x-=1024;
    if(anchor-x>512) x+=1024;
    return x;
}
static void presentation_line(Ppu *p,int line,bool begin) {
    if(!begin) {
        if(remapped) {
            memcpy(p->hScroll,original_scroll,sizeof(original_scroll));
        }
        p->widescreenResolveAllTiles=false;
        p->widescreenMode7Native=false;
        p->widescreenMode7Shift=0;
        p->widescreenOamXOffset=0;
        PpuSetWidescreenLayerElasticBand(p,2,0,0,NULL,0);
        PpuSetWidescreenBg3Widen(p,0);
        static const uint8_t no_hints[16]={0};
        PpuWsSetOamRightHints(p,no_hints);
        remapped=0;
        return;
    }
    line_shift=0;remapped=0;
    if(line<=32 || (PPU_mode(p)!=1 && PPU_mode(p)!=7)) return;
    memcpy(original_scroll,p->hScroll,sizeof(original_scroll));
    line_camera=camera_x(original_scroll[0]);
    int extra=p->extraLeftRight;
    int offset=sciv_widescreen_view_offset(extra,line_camera,section_first,section_last);
    line_shift=extra-offset;
    p->widescreenMode7Native=PPU_mode(p)==7;
    p->widescreenMode7Shift=line_shift;
    p->widescreenOamXOffset=-line_shift;
    if(line==33 && getenv("SCIV_WIDESCREEN_CAMERA_TRACE"))
        fprintf(stderr,"[wide-view] frame=%d camera=%d offset=%d first=%d last=%d\n",snes_frame_counter,line_camera,offset,section_first,section_last);
    if(!line_shift) return;
    /* BG3 also carries world foliage in some rooms. Move its authentic
     * pixels with the field, while leaving unverified extra foliage blank. */
    PpuWsElasticSeg foliage={0,256,-line_shift,256-line_shift};
    PpuSetWidescreenBg3Widen(p,33);
    PpuSetWidescreenLayerElasticBand(p,2,33,225,&foliage,1);
    /* Keep each raster line's native BG2 phase/HDMA difference. SCIV's
     * first-stage backdrop scrolls at BG1 speed, unlike SMW's half-speed BG. */
    p->hScroll[0]=(original_scroll[0]+line_shift)&1023;
    p->hScroll[1]=(original_scroll[1]+line_shift)&1023;
    p->widescreenResolveAllTiles=true;
    remapped=1;
}
static uint16_t native_tile(const Ppu *p,int layer,int x,unsigned sy) {
    int vx=(x+(remapped?original_scroll[layer]:p->hScroll[layer]))&1023;
    unsigned addr=PPU_bgTilemapAdr(p,layer)+((sy>>3)&31)*32+((vx>>3)&31);
    if((vx&256) && PPU_bgTilemapWider(p,layer)) addr+=1024;
    if((sy&256) && PPU_bgTilemapHigher(p,layer))
        addr+=PPU_bgTilemapWider(p,layer)?2048:1024;
    return PpuRenderVram(p)[addr&32767];
}
static uint32_t resolve(const Ppu *p,int layer,int sx,uint32_t sy,void *ctx) {
    (void)ctx;
    if(layer<0 || layer>1) return 0x10000;
    int source_x=sx+line_shift;
    int foreground=camera_x(remapped?original_scroll[0]:p->hScroll[0]);
    int first=section_first,last=section_last;
    int world=foreground+source_x;
    if(first<=last && (world+7<first || world>=last+256)) return 0x10000;
    if(source_x>=-7 && source_x<256)
        return native_tile(p,layer,source_x,sy);
    Row *row=&rows[layer];
    unsigned scroll=remapped?original_scroll[layer]:p->hScroll[layer];
    unsigned map=PPU_bgTilemapAdr(p,layer), chars=PPU_bgTileAdr(p,layer);
    int camera=camera_x(scroll);
    if(!row->checked || row->y!=sy || row->scroll!=scroll || row->map!=map || row->chars!=chars) {
        *row=(Row){sy,scroll,map,chars,1,0,0};
        if(PPU_mode(p)==1 && !PPU_bigTiles(p,layer) && !PPU_mosaicEnabled(p,layer)) {
            int matches=0, repeated=0;
            int period=PPU_bgTilemapWider(p,layer)?512:256;
            for(int x=0;x<256;x+=8) {
                uint16_t t;
                int wx=camera+x;
                if(sciv_widescreen_room_tile(g_ram,layer,pages_w,pages_h,wx,sy&1023,&t)
                    && t==native_tile(p,layer,x,sy)) matches++;
                if(layer==1 && sciv_widescreen_room_tile(g_ram,layer,pages_w,pages_h,
                    (wx%period+period)%period,sy&1023,&t) && t==native_tile(p,layer,x,sy)) repeated++;
            }
            /* Section 1-1's BG2 contains BOTH the periodic distant cliffs
             * and the world-addressed ledge/pit strip below map row 192.
             * At the entrance both hypotheses match; choose its known split
             * before the distant tiles have entered the native viewport. */
            if(layer==1) {
                if(word(g_ram,0x86)==0) row->periodic=(sy&1023)<192;
                else row->periodic=repeated>=matches;
                if(row->periodic) matches=repeated;
            }
            /* A local dynamic tile upload must not hide the entire margin
             * scanline. Still reject layouts with widespread disagreement. */
            row->valid=matches>=24;
        }
    }
    uint16_t entry;
    int wx=camera+source_x;
    if(row->periodic) {
        int period=PPU_bgTilemapWider(p,layer)?512:256;
        wx=(wx%period+period)%period;
    }
    if(row->valid && sciv_widescreen_room_tile(g_ram,layer,pages_w,pages_h,
                         wx,sy&1023,&entry)) return entry;
    return 0x10000;
}
int sciv_widescreen_tiles_begin(Ppu *p) {
    memset(rows,0,sizeof(rows));
    p->widescreenPresentationLine=NULL;
    p->widescreenResolveAllTiles=false;
    p->widescreenMode7Native=false;
    p->widescreenMode7Shift=p->widescreenOamXOffset=0;
    line_shift=remapped=0;
    PpuSetWidescreenTileResolver(p,NULL,NULL);
    /* End-of-CPU registers can describe the next split (often Mode 7).
     * Resolve only after the raster journal has selected the actual line. */
    if(!g_snes || !g_snes->cart) return 0;
    unsigned room=word(g_ram,0x86);
    if(room>=68 || g_snes->cart->romSize<0x30320) return 0;
    /* LoROM $86:8296: room layout handler, not the stage number. */
    unsigned type=word(g_snes->cart->rom,0x30296+room*2);
    /* USA $85:CFD2/$85:D05A are the initial room camera limits (SC4Ed).
     * Live limits can shrink to a one-way lock, notably at the drawbridge.
     * Combine them without history so rewind/load/resize stay deterministic. */
    section_first=word(g_snes->cart->rom,0x2cfd2+room*2);
    section_last=word(g_snes->cart->rom,0x2d05a+room*2);
    int live_first=word(g_ram,0xa0),live_last=word(g_ram,0xa2);
    if(live_first<section_first) section_first=live_first;
    if(live_last>section_last) section_last=live_last;
    static unsigned frames;
    if(getenv("SCIV_WIDESCREEN_DIAGNOSTICS") && frames++<4)
        fprintf(stderr,"[wide-map] room=%u type=%u mode=%u\n",room,type,PPU_mode(p));
    switch(type) {
        case 0: pages_w=8; pages_h=2; break;
        case 1: case 12: pages_w=32; pages_h=2; break;
        case 2: case 10: case 11: case 13: pages_w=16; pages_h=4; break;
        case 3: pages_w=2; pages_h=32; break;
        case 6: case 9: case 14: case 16: pages_w=16; pages_h=1; break;
        case 7: case 8: case 17: case 18: pages_w=1; pages_h=16; break;
        default: return 0;
    }
    PpuSetWidescreenTileResolver(p,resolve,NULL);
    p->widescreenPresentationLine=presentation_line;
    return 1;
}
