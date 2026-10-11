#define SDL_MAIN_HANDLED 1
#include "../src/sciv_interpolation.c"
#include <assert.h>
uint8 g_ram[0x20000];
Ppu *g_ppu;
Config g_config;
static Ppu live, saved;
static int drawn, expect_x, expect_y, expect_scroll, check_bg3, expect_bg3;
void PpuBeginDrawing(Ppu *p,uint8_t *dst,size_t pitch,uint32_t flags) {p->renderBuffer=dst;p->renderPitch=pitch;p->renderFlags=flags;}
void ppu_runLine(Ppu *p,int line) {
    drawn++;
    if(line<=32 && check_bg3)assert(p->hScroll[2]==8);
    if(line==41) {
        assert((p->oam[0]&255)==expect_x);
        assert((p->oam[0]>>8)==expect_y);
        assert(p->hScroll[0]==expect_scroll);
        if(check_bg3)assert(p->hScroll[2]==expect_bg3);
    }
}
void snes_beam_frame_driver_draw_ppu_frame(void) {}
void snes_beam_frame_driver_draw_ppu_frame_observed(SnesBeamRasterObserver *observer,void *ctx) {
    for(unsigned y=1;y<=224;y++)observer(g_ppu,y,ctx);
}
static void test_rotating_ledge_slots(void) {
    Motion old={0};Scanline current={0};
    for(int i=0;i<128;i++){old.oam[i*2]=current.motion.oam[i*2]=240u<<8;}
    old.oam[0]=(80<<8)|80;old.oam[2]=(80<<8)|96;
    current.motion.oam[0]=(80<<8)|96;current.motion.oam[2]=(80<<8)|80;
    old.oam[1]=old.oam[3]=current.motion.oam[1]=current.motion.oam[3]=7;
    match_sprites(&current,&old);
    assert(current.previous_slot[0]==1 && current.previous_slot[1]==0);
    /* The camera moved four pixels; world positions are still unchanged. */
    uint16_t scroll=4;memcpy(current.motion.regs+offsetof(Ppu,hScroll),&scroll,2);
    current.motion.oam[0]=(80<<8)|92;current.motion.oam[2]=(80<<8)|76;
    match_sprites(&current,&old);
    assert(current.previous_slot[0]==1 && current.previous_slot[1]==0);
    /* Ambiguous overlapping copies must snap rather than invent identity. */
    old.oam[2]=old.oam[0];current.motion.oam[0]=current.motion.oam[2];
    match_sprites(&current,&old);assert(current.previous_slot[0]==-1);
}
int main(void) {
    test_rotating_ledge_slots();
    assert(position(2,510,512,16,0)==510);
    assert(position(2,510,512,16,0.5)==0);
    assert(position(2,510,512,16,1)==2);
    assert(position(100,0,512,16,0.5)==100);
    g_ppu=&live; live.bgmode=1;live.inidisp=15;g_ram[0x70]=5;
    live.oam[0]=(40<<8)|40;live.oam[1]=7;live.hScroll[0]=20;
    enabled=0;g_config.frame_blend=true;
    assert(sciv_interpolation_hz(144)==0 && g_config.frame_blend);
    enabled=1;assert(sciv_interpolation_hz(144)==144 && !g_config.frame_blend);
    g_config.frame_blend=true;sciv_interpolation_raster();assert(!valid && !g_config.frame_blend);
    live.oam[0]=(44<<8)|44;live.hScroll[0]=24;
    sciv_interpolation_raster();assert(valid);saved=live;
    uint8_t output[4];expect_x=42;expect_y=42;expect_scroll=22;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));assert(drawn==224);
    assert(!memcmp(&live,&saved,sizeof(live)));
    expect_x=40;expect_y=40;expect_scroll=20;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0));
    live.oam[1]=8;sciv_interpolation_raster();
    expect_x=44;expect_y=44;expect_scroll=24;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    /* A new/changed sprite must share terrain's delayed camera. */
    live.oam[1]=9;live.hScroll[0]=32;live.vScroll[0]=8;
    sciv_interpolation_raster();expect_x=48;expect_y=48;expect_scroll=28;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    /* Newly visible slots get camera compensation, never old parked motion. */
    live.oam[0]=(240<<8)|44;sciv_interpolation_raster();
    live.oam[0]=(44<<8)|44;live.hScroll[0]=40;
    sciv_interpolation_raster();expect_x=48;expect_y=44;expect_scroll=36;
    saved=live;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    assert(!memcmp(&live,&saved,sizeof(live)));
    /* Stable tile numbers with replaced sprite artwork must snap motion. */
    sciv_interpolation_raster();
    live.vram[7*16]=1;live.oam[1]=7;sciv_interpolation_raster();
    live.vram[7*16]=2;live.oam[0]=(48<<8)|48;
    sciv_interpolation_raster();expect_x=48;expect_y=48;expect_scroll=40;
    /* Keep this artwork outside the background maps. */
    assert(geometry_changed); /* same memory is a BG map in this fixture */
    assert(!sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    live.bgXsc[0]=live.bgXsc[1]=live.bgXsc[2]=0x40;sciv_interpolation_raster();
    sciv_interpolation_raster();live.vram[7*16]=3;live.oam[0]=(52<<8)|52;
    sciv_interpolation_raster();expect_x=52;expect_y=52;
    assert(!geometry_changed);
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    /* Crumbling terrain changes force a whole authentic frame. */
    live.vram[0x4000]=1;sciv_interpolation_raster();assert(geometry_changed);
    assert(!sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    sciv_interpolation_raster();assert(!geometry_changed);
    /* BG3 world graphics must move with the other world layers below HUD. */
    sciv_interpolation_raster();live.hScroll[2]=8;
    sciv_interpolation_raster();check_bg3=1;expect_bg3=4;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    check_bg3=0;
    /* A collapse authored on BG3 must trigger the terrain safeguard too. */
    live.bgXsc[2]=0x50;sciv_interpolation_raster();sciv_interpolation_raster();
    live.vram[0x5000]=1;sciv_interpolation_raster();assert(geometry_changed);
    assert(!sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    g_ram[0x86]=1;sciv_interpolation_raster();assert(!valid);
    sciv_interpolation_reset();assert(!sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    /* Exercise replay itself when identical ledge tiles exchange OAM slots. */
    memset(&live,0,sizeof(live));live.bgmode=1;live.inidisp=15;
    for(int i=0;i<128;i++)live.oam[i*2]=240u<<8;
    live.oam[0]=(80<<8)|80;live.oam[2]=(80<<8)|96;
    live.oam[1]=live.oam[3]=7;sciv_interpolation_reset();sciv_interpolation_raster();
    live.oam[0]=(80<<8)|96;live.oam[2]=(80<<8)|80;
    sciv_interpolation_raster();expect_x=96;expect_y=80;expect_scroll=0;saved=live;
    assert(sciv_interpolation_draw(output,1024,NULL,256,224,0.5));
    assert(!memcmp(&live,&saved,sizeof(live)));
    puts("Interpolation passed: wrapping, endpoints, slot changes, room reset, guest isolation.");
}









