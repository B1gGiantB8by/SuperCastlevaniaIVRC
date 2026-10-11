/* Presentation-only PPU replay. Guest CPU, DMA, RAM and live PPU are never
 * stepped or edited by an intermediate presentation. Current tile/palette
 * artwork is discrete; only compatible OAM positions and scrolls move. */
#include "sciv_interpolation.h"
#include "common_rtl.h"
#include "desktop/config.h"
#include "snes/ppu.h"
#include "beam_frame_driver.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t regs[PPU_SAVESTATE_REGS_SIZE];
    uint16_t oam[256];
    uint8_t high[32];
    uint32_t terrain[3], artwork[128];
} Motion;
typedef struct {
    Motion motion;
    int16_t previous_slot[128];
    uint16_t palette[256], vram[32768];
} Scanline;
static Scanline *lines;
static Motion previous[224];
static Ppu *scratch;
static Ppu old_regs;
static int enabled, valid, captured, room = -1;
static int geometry_changed;
static int verify_replay;

void sciv_interpolation_reset(void) { valid=captured=0; room=-1; }
int sciv_interpolation_enabled(void) { return enabled; }
void sciv_interpolation_init(void) {
    FILE *f=fopen("interpolation.cfg","r"); int value=0;
    if(f) { if(fscanf(f,"%d",&value)!=1)value=0; fclose(f); }
    const char *override=getenv("SCIV_INTERPOLATION");
    if(override)value=atoi(override);
    enabled=value==1;
    if(enabled)g_config.frame_blend=false;
    sciv_interpolation_reset();
}
void sciv_interpolation_set(int value) {
    enabled=!!value;
    if(enabled)g_config.frame_blend=false;
    sciv_interpolation_reset();
    FILE *f=fopen("interpolation.cfg","w");
    if(f) {fprintf(f,"%d\n",enabled);fclose(f);}
}
double sciv_interpolation_hz(double refresh) {
    /* The host polls the requested rate when settings change. */
    if(!enabled)return 0;
    /* Temporal averaging after positional interpolation creates sprite trails.
     * Also enforce after the advanced launcher reloads shared settings. */
    g_config.frame_blend=false;
    return refresh>60.1 ? refresh : 120.1976;
}
static uint32_t hash_words(const uint16_t *vram,unsigned address,unsigned count) {
    uint32_t h=2166136261u;
    for(unsigned i=0;i<count;i++)h=(h^vram[(address+i)&32767])*16777619u;
    return h;
}
static uint32_t sprite_artwork(const Ppu *ppu,int slot) {
    static const uint8_t sizes[8][2]={{8,16},{8,32},{8,64},{16,32},{16,64},{32,64},{16,32},{16,32}};
    int attr=ppu->oam[slot*2+1];
    int big=(ppu->highOam[slot>>2]>>((slot&3)*2+1))&1;
    int tiles=sizes[PPU_objSize(ppu)][big]/8;
    unsigned base=(attr&256) ? PPU_objTileAdr2(ppu) : PPU_objTileAdr1(ppu);
    uint32_t h=2166136261u;
    for(int row=0;row<tiles;row++)for(int col=0;col<tiles;col++) {
        unsigned tile=(((attr&255)/16+row)<<4)|(((attr&15)+col)&15);
        h=(h^hash_words(ppu->vram,base+tile*16,16))*16777619u;
    }
    return h;
}
static int delta(int cur,int old,int period);
static unsigned scroll_word(const Motion *m,size_t offset) {
    uint16_t value;memcpy(&value,m->regs+offset,sizeof(value));return value;
}
static int sprite_x(const Motion *m,int slot) {
    return (m->oam[slot*2]&255)|(((m->high[slot>>2]>>((slot&3)*2))&1)<<8);
}
/* OAM slots are draw order, not object identities. Require a unique, mutual
 * nearest artwork match in world coordinates, so a rotating table of equal
 * ledge tiles cannot interpolate one stationary tile toward its neighbour. */
static void match_sprites(Scanline *s,const Motion *old) {
    int nearest[128],reverse[128],best[128],other_best[128],ties[128],other_ties[128];
    for(int i=0;i<128;i++) {
        s->previous_slot[i]=-1;nearest[i]=reverse[i]=-1;
        best[i]=other_best[i]=100000;ties[i]=other_ties[i]=0;
    }
    int camera_x=delta(scroll_word(&s->motion,offsetof(Ppu,hScroll)),scroll_word(old,offsetof(Ppu,hScroll)),1024);
    int camera_y=delta(scroll_word(&s->motion,offsetof(Ppu,vScroll)),scroll_word(old,offsetof(Ppu,vScroll)),1024);
    for(int c=0;c<128;c++) {
        int cy=s->motion.oam[c*2]>>8;if(cy>=224)continue;
        int cx=sprite_x(&s->motion,c);
        int size=(s->motion.high[c>>2]>>((c&3)*2+1))&1;
        for(int p=0;p<128;p++) {
            int py=old->oam[p*2]>>8;
            if(py>=224 || s->motion.oam[c*2+1]!=old->oam[p*2+1] ||
               s->motion.artwork[c]!=old->artwork[p] ||
               size!=((old->high[p>>2]>>((p&3)*2+1))&1))continue;
            int dx=abs(delta(cx+camera_x,sprite_x(old,p),512));
            int dy=abs(delta(cy+camera_y,py,256));
            if(dx>16 || dy>16)continue;
            int distance=dx*dx+dy*dy;
            if(distance<best[c]) {best[c]=distance;nearest[c]=p;ties[c]=1;}
            else if(distance==best[c])ties[c]++;
            if(distance<other_best[p]) {other_best[p]=distance;reverse[p]=c;other_ties[p]=1;}
            else if(distance==other_best[p])other_ties[p]++;
        }
    }
    for(int c=0;c<128;c++) {
        int p=nearest[c];
        if(p>=0 && ties[c]==1 && other_ties[p]==1 && reverse[p]==c)s->previous_slot[c]=(int16_t)p;
    }
}
static void capture(const Ppu *ppu,unsigned line,void *context) {
    (void)context;
    Scanline *s=&lines[line-1];
    memcpy(s->motion.regs,ppu, sizeof(s->motion.regs));
    memcpy(s->motion.oam,ppu->oam,sizeof(s->motion.oam));
    memcpy(s->motion.high,ppu->highOam,sizeof(s->motion.high));
    memcpy(s->palette,ppu->cgram,sizeof(s->palette));
    memcpy(s->vram,ppu->vram,sizeof(s->vram));
    for(int layer=0;layer<3;layer++) {
        unsigned count=1024u<<((ppu->bgXsc[layer]&1)+((ppu->bgXsc[layer]>>1)&1));
        s->motion.terrain[layer]=hash_words(ppu->vram,PPU_bgTilemapAdr(ppu,layer),count);
        if(valid && line>32 && s->motion.terrain[layer]!=previous[line-1].terrain[layer])
            geometry_changed=1;
    }
    for(int slot=0;slot<128;slot++)
        s->motion.artwork[slot]=sprite_artwork(ppu,slot);
    if(valid) {
        if(line>1 && !memcmp(&s->motion,&lines[line-2].motion,sizeof(Motion)) &&
           !memcmp(&previous[line-1],&previous[line-2],sizeof(Motion)))
            memcpy(s->previous_slot,lines[line-2].previous_slot,sizeof(s->previous_slot));
        else match_sprites(s,&previous[line-1]);
    }
}
void sciv_interpolation_raster(void) {
    if(enabled)g_config.frame_blend=false;
    int next_room=g_ram[0x86]|(g_ram[0x87]<<8);
    if(!enabled || g_ram[0x70]!=5 || g_ram[0x71] ||
       g_ppu->extraLeftRight || PPU_mode(g_ppu)==7) {
        sciv_interpolation_reset();
        snes_beam_frame_driver_draw_ppu_frame(); return;
    }
    if(!lines)lines=malloc(224*sizeof(*lines));
    if(!scratch)scratch=malloc(sizeof(*scratch));
    if(!lines || !scratch) {sciv_interpolation_reset();snes_beam_frame_driver_draw_ppu_frame();return;}
    valid=captured && room==next_room;
    if(valid)for(int i=0;i<224;i++)previous[i]=lines[i].motion;
    geometry_changed=0;
    snes_beam_frame_driver_draw_ppu_frame_observed(capture,NULL);
    *scratch=*g_ppu; captured=1; room=next_room;
    if(getenv("SCIV_INTERPOLATION_VERIFY")) {
        static uint32_t exact[256*224];
        verify_replay=1;
        if(sciv_interpolation_draw((uint8_t *)exact,256*4,NULL,256,224,1)) {
            for(int y=0;y<224;y++)if(memcmp(exact+y*256,
                    g_ppu->renderBuffer+y*g_ppu->renderPitch,256*4)) {
                fprintf(stderr,"[interpolation-verify] native mismatch line=%d\n",y);
                break;
            }
        }
        verify_replay=0;
    }
}
static int delta(int cur,int old,int period) {
    int d=(cur-old)&(period-1);
    return d>=period/2 ? d-period : d;
}
static int position(int cur,int old,int period,int limit,double alpha) {
    int d=delta(cur,old,period);
    if(abs(d)>limit)return cur;
    return (cur-(int)lround(d*(1-alpha)))&(period-1);
}
int sciv_interpolation_draw(uint8_t *dst,size_t pitch,const uint8_t *field,
                            int width,int height,double alpha) {
    (void)field;
    if(!enabled || (!verify_replay && (!valid || geometry_changed || alpha>=1)) || width!=256 || height!=224)return 0;
    if(alpha<0)alpha=0;
    PpuBeginDrawing(scratch,dst,pitch,g_ppu->renderFlags);
    scratch->mode7Hd.pixels=NULL;
    scratch->renderVram=NULL;
    scratch->extraObjectCount=0;
    scratch->widescreenLineEnhancer=NULL;
    scratch->widescreenTileResolver=NULL;
    memset(scratch->overlayRenderBuffer,0,sizeof(scratch->overlayRenderBuffer));
    scratch->widescreenPresentationLine=NULL;
    for(int y=0;y<224;y++) {
        Scanline *s=&lines[y]; Motion *old=&previous[y];
        memcpy(scratch,s->motion.regs,sizeof(s->motion.regs));
        memcpy(scratch->oam,s->motion.oam,sizeof(s->motion.oam));
        memcpy(scratch->highOam,s->motion.high,sizeof(s->motion.high));
        memcpy(scratch->cgram,s->palette,sizeof(s->palette));
        memcpy(scratch->vram,s->vram,sizeof(s->vram));
        /* Preserve status strip, raster effects, mode/tileset changes and fades. */
        memcpy(&old_regs,old->regs,sizeof(old->regs));
        int compatible=y>=32 && !PPU_forcedBlank(scratch) &&
            scratch->inidisp==old_regs.inidisp && scratch->bgmode==old_regs.bgmode &&
            scratch->bgTileAdr==old_regs.bgTileAdr && scratch->obsel==old_regs.obsel &&
            PPU_mode(scratch)==1 && !scratch->mosaic && !scratch->setini;
        if(compatible) {
            int camera_x=scratch->hScroll[0], camera_y=scratch->vScroll[0];
            for(int layer=0;layer<3;layer++)if(scratch->bgXsc[layer]==old_regs.bgXsc[layer]) {
                scratch->hScroll[layer]=position(scratch->hScroll[layer],old_regs.hScroll[layer],1024,16,alpha);
                scratch->vScroll[layer]=position(scratch->vScroll[layer],old_regs.vScroll[layer],1024,16,alpha);
            }
            int camera_dx=delta(camera_x,scratch->hScroll[0],1024);
            int camera_dy=delta(camera_y,scratch->vScroll[0],1024);
            for(int slot=0;slot<128;slot++) {
                int index=slot*2,shift=(slot&3)*2,hi=slot>>2;
                int ch=(s->motion.high[hi]>>shift)&3;
                int cy=s->motion.oam[index]>>8;
                int matched=s->previous_slot[slot];
                int py=matched>=0 ? old->oam[matched*2]>>8 : cy;
                int cx=(s->motion.oam[index]&255)|((ch&1)<<8);
                int px=matched>=0 ? sprite_x(old,matched) : cx;
                /* Even a new sprite uses the same delayed camera as terrain.
                 * Leaving unmatched slots at current screen coordinates makes
                 * spawns / animation changes slide into an interpolated ledge. */
                int x=(cx+camera_dx)&511, yy=(cy+camera_dy)&255;
                if(cy>=224)continue; /* hidden OAM must remain hidden */
                int tracked=matched>=0;
                if(tracked) {
                    x=position(cx,px,512,16,alpha);
                    yy=position(cy,py,256,16,alpha);
                }
                scratch->oam[index]=(uint16_t)((yy<<8)|(x&255));
                scratch->highOam[hi]=(scratch->highOam[hi]&~(1<<shift))|(((x>>8)&1)<<shift);
            }
        }
        ppu_runLine(scratch,y+1);
    }
    return 1;
}









