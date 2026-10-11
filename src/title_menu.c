#include "cheats.h"
#include "sciv_interpolation.h"
#include "title_menu.h"
extern void sciv_open_audio_settings(void);
#include "common_rtl.h"
#include "snes_overlay_draw.h"
#include "achievement_overlay.h"
#include "achievements.h"
#include "boss_rush.h"
#include "level_select.h"
#include "other_castle.h"
#include "game_timing.h"
#include "bezel.h"
#include "desktop/config.h"
#include "snes/ppu.h"
#include <stdio.h>
#include <string.h>

enum { ROOT, MODES, LEVEL, ACHIEVEMENTS, QUIT, OPTIONS, VIDEO, AUDIO, GAMEPLAY, CONTROLS, REWIND, CHEATS };
enum { NONE, FULLSCREEN, SCALE, ASPECT, FILTER, VSYNC, BLEND, INTERPOLATION, SHADER, RENDERER, SPRITES, BEZEL, VOLUME, AUDIO_ON, TIMING, ACH_ON, SOURCE, DEADZONE, RUMBLE, REWIND_ON, DEPTH, INTERVAL, RUNAHEAD, LAUNCHER, CHEAT_LIVES, CHEAT_INVINCIBLE, CHEAT_HEARTS, CHEAT_TIMER };
typedef struct { const char *label; int page,key,min,max,step; } Item;
#define LINK(s,p) {s,p,NONE,0,0,0}
#define VALUE(s,k,lo,hi,st) {s,-1,k,lo,hi,st}
static const Item root[]={LINK("START GAME",-2),LINK("PASSWORD",-3),LINK("OPTIONS",OPTIONS),LINK("ADDITIONAL MODES",MODES),LINK("ACHIEVEMENTS",ACHIEVEMENTS),LINK("QUIT",QUIT),LINK("CHEATS / GAME GENIE",CHEATS)};
static const Item modes[]={LINK("BOSS RUSH",-4),LINK("ONE LIFE",-5),LINK("LEVEL SELECT",LEVEL),LINK("BACK",ROOT)};
static const Item quit_items[]={LINK("CANCEL",ROOT),LINK("QUIT TO DESKTOP",-6)};
static const Item options[]={LINK("VIDEO",VIDEO),LINK("AUDIO",AUDIO),LINK("GAMEPLAY",GAMEPLAY),LINK("CONTROLS",CONTROLS),LINK("REWIND / LATENCY",REWIND),LINK("CHEATS / GAME GENIE",CHEATS),LINK("ADVANCED SETTINGS",-7),LINK("BACK",ROOT)};
static const Item video[]={VALUE("SCREEN",FULLSCREEN,0,2,1),VALUE("WINDOW SCALE",SCALE,1,8,1),VALUE("ASPECT",ASPECT,0,2,1),VALUE("SMOOTHING",FILTER,0,1,1),VALUE("VSYNC",VSYNC,0,2,1),VALUE("FRAME BLEND",BLEND,0,1,1),VALUE("MOTION (EXPERIMENTAL)",INTERPOLATION,0,1,1),VALUE("SHADER",SHADER,0,4,1),VALUE("RENDERER",RENDERER,0,2,1),VALUE("SPRITE LIMITS",SPRITES,0,1,1),VALUE("BEZEL",BEZEL,0,1,1),LINK("BACK",OPTIONS)};
static const Item audio[]={VALUE("AUDIO",AUDIO_ON,0,1,1),VALUE("VOLUME",VOLUME,0,100,5),LINK("MSU-1 SOUNDTRACK",-7),LINK("BACK",OPTIONS)};
static const Item gameplay[]={VALUE("GAME TIMING",TIMING,0,1,1),VALUE("ACHIEVEMENTS",ACH_ON,0,1,1),VALUE("SKIP LAUNCHER",LAUNCHER,0,1,1),LINK("BACK",OPTIONS)};
static const Item controls[]={VALUE("PLAYER 1",SOURCE,0,2,1),VALUE("DEADZONE %",DEADZONE,0,50,5),VALUE("RUMBLE",RUMBLE,0,1,1),LINK("REMAP CONTROLS",-7),LINK("BACK",OPTIONS)};
static const Item rewind_items[]={VALUE("REWIND",REWIND_ON,0,1,1),VALUE("HISTORY",DEPTH,60,600,60),VALUE("FRAME INTERVAL",INTERVAL,1,6,1),VALUE("RUN AHEAD",RUNAHEAD,0,3,1),LINK("BACK",OPTIONS)};
static const Item cheats[]={VALUE("INFINITE LIVES",CHEAT_LIVES,0,1,1),VALUE("INVINCIBILITY",CHEAT_INVINCIBLE,0,1,1),VALUE("START WITH 98 HEARTS",CHEAT_HEARTS,0,1,1),VALUE("STOP TIMER",CHEAT_TIMER,0,1,1),LINK("BACK",ROOT)};
static const char *shader_paths[]={"","assets/shaders/crt-soft.glslp","assets/shaders/lcd-grid.glslp","assets/shaders/sharp.glslp","assets/shaders/warm-composite.glslp"};
static int page,selected,visible,handoff,release,repeat,settle,enabled=1;
static int departing;
static uint32_t previous;
static unsigned word(unsigned p) { return g_ram[p]|(g_ram[p+1]<<8); }
void sciv_title_enable(int value) { enabled=!!value; }
void sciv_title_reset(void) { page=selected=visible=handoff=release=repeat=settle=departing=0; previous=0; }
static void go(int p) { page=p; selected=repeat=0; }
static const Item *items(int *n) {
#define LIST(p,a) case p: *n=sizeof(a)/sizeof(a[0]); return a
    switch(page) { LIST(CHEATS,cheats); LIST(ROOT,root); LIST(MODES,modes); LIST(QUIT,quit_items); LIST(OPTIONS,options); LIST(VIDEO,video); LIST(AUDIO,audio); LIST(GAMEPLAY,gameplay); LIST(CONTROLS,controls); LIST(REWIND,rewind_items); default: *n=page==LEVEL ? sciv_level_select_count()+1 : page==ACHIEVEMENTS ? SCIV_ACHIEVEMENT_COUNT : 1; return NULL; }
#undef LIST
}
static int get(int key) {
    if(key>=CHEAT_LIVES && key<=CHEAT_TIMER)return sciv_cheat_get(key-CHEAT_LIVES);
    switch(key) {
    case FULLSCREEN:return g_config.fullscreen; case SCALE:return g_config.window_scale ? g_config.window_scale : 3;
    case ASPECT:return g_config.display_aspect; case FILTER:return g_config.linear_filtering; case VSYNC:return g_config.vsync; case BLEND:return g_config.frame_blend;
    case INTERPOLATION:return sciv_interpolation_enabled();
    case SHADER:for(int i=0;i<5;i++) if(!strcmp(g_config.shader ? g_config.shader : "",shader_paths[i])) return i; return 0;
    case RENDERER:return g_config.output_method; case SPRITES:return !g_config.no_sprite_limits; case BEZEL:return sciv_bezel_enabled();
    case VOLUME:return g_config.volume; case AUDIO_ON:return g_config.enable_audio; case TIMING:return sciv_timing_enhanced(); case ACH_ON:return sciv_achievements_enabled();
    case RUMBLE:return g_config.enable_rumble; case SOURCE:return g_config.player_src[0]; case DEADZONE:return (g_config.gamepad_deadzone*100+16383)/32767;
    case REWIND_ON:return g_config.rewind_enabled; case DEPTH:return g_config.rewind_depth; case INTERVAL:return g_config.rewind_interval; case RUNAHEAD:return g_config.run_ahead;
    case LAUNCHER:return g_config.skip_launcher; default:return 0;
    }
}
static void set(int key,int v) {
    if(key>=CHEAT_LIVES && key<=CHEAT_TIMER){sciv_cheat_set(key-CHEAT_LIVES,v);return;}
    switch(key) {
    case FULLSCREEN:g_config.fullscreen=v;break; case SCALE:g_config.window_scale=v;g_config.window_width=g_config.window_height=0;break;
    case ASPECT:g_config.display_aspect=v;g_config.ignore_aspect_ratio=0;break; case FILTER:g_config.linear_filtering=v;break; case VSYNC:g_config.vsync=v;break; case BLEND:if(v && sciv_interpolation_enabled())sciv_interpolation_set(0);g_config.frame_blend=v;break;
    case INTERPOLATION:sciv_interpolation_set(v);break;
    case SHADER:g_config.shader=v ? shader_paths[v] : NULL;if(v) {g_config.output_method=kOutputMethod_OpenGL;strcpy(g_config.renderer,"opengl");}break;
    case RENDERER:g_config.output_method=v;strcpy(g_config.renderer,v==2 ? "opengl" : v==1 ? "software" : "auto");if(v!=2) g_config.shader=NULL;break;
    case SPRITES:g_config.no_sprite_limits=!v;break; case BEZEL:sciv_bezel_set(v);break;
    case VOLUME:g_config.volume=v;break;case AUDIO_ON:g_config.enable_audio=v;break; case TIMING:sciv_timing_set(v);break;case ACH_ON:sciv_achievements_set_enabled(v);break;
    case RUMBLE:g_config.enable_rumble=v;break;case SOURCE:g_config.player_src[0]=v;g_config.enable_gamepad[0]=v==2;break;case DEADZONE:g_config.gamepad_deadzone=v*32767/100;break;
    case REWIND_ON:g_config.rewind_enabled=v;break;case DEPTH:g_config.rewind_depth=v;break;case INTERVAL:g_config.rewind_interval=v;break;case RUNAHEAD:g_config.run_ahead=v;break;case LAUNCHER:g_config.skip_launcher=v;break;
    }
}
static void value_label(int key,char *out,size_t cap) {
    int v=get(key); const char *s=NULL;
    static const char *screen[]={"WINDOWED","BORDERLESS","FULLSCREEN"},*aspect[]={"4:3","8:7","1:1"},*vsync[]={"OFF","ON","ADAPTIVE"},*shader[]={"NONE","CRT SOFT","LCD GRID","SHARP","WARM COMPOSITE"},*renderer[]={"SDL","SOFTWARE","OPENGL"},*source[]={"NONE","KEYBOARD","GAMEPAD"};
    if(key==FULLSCREEN && v>=0 && v<3)s=screen[v];else if(key==ASPECT && v>=0 && v<3)s=aspect[v];else if(key==VSYNC && v>=0 && v<3)s=vsync[v];else if(key==SHADER)s=shader[v];else if(key==RENDERER && v>=0 && v<3)s=renderer[v];else if(key==SOURCE && v>=0 && v<3)s=source[v];else if(key==TIMING)s=v ? "ENHANCED" : "ORIGINAL";
    else if(key==DEPTH) {snprintf(out,cap,"%d SEC",v*g_config.rewind_interval/60);return;}
    else if(key==SCALE || key==VOLUME || key==DEADZONE || key==INTERVAL || key==RUNAHEAD) {snprintf(out,cap,"%d%s",v,key==SCALE ? "X" : key==VOLUME || key==DEADZONE ? "%" : "");return;}
    else if(key==LAUNCHER)s=v ? "[X]" : "[ ]";
    else s=v ? "ON" : "OFF";
    snprintf(out,cap,"%s",s ? s : "CUSTOM");
}
uint32_t sciv_title_input(uint32_t input,int *action,int *freeze) {
    *action=*freeze=0;
    if(handoff) {int step=(12-handoff)/4;uint32_t result=step==2 ? SNES_PAD_START : step==0 ? SNES_PAD_DOWN : 0;handoff--;return result;}
    if(!enabled || sciv_other_castle_active() || word(0x32)!=1 || word(0x34)!=1) {visible=0;previous=input;release=settle=0;return input;}
    if(release) {visible=0;return 0;}
    if(!visible) {go(ROOT);visible=1;departing=0;previous=input;}
    /* Paint immediately, but let the cartridge finish its fade/input setup.
     * Eat presses during this interval so the hidden native menu cannot move. */
    if(settle<60) {settle++;previous=input;return 0;}
    *freeze=1;
    uint32_t pressed=input & ~previous;
    uint32_t nav=input & (SNES_PAD_UP|SNES_PAD_DOWN|SNES_PAD_LEFT|SNES_PAD_RIGHT);
    if(nav) {if(input==previous) {repeat++;if(repeat>=21 && (repeat-21)%6==0)pressed|=nav;}else repeat=0;}else repeat=0;
    previous=input;
    int n;const Item *list=items(&n);
    if(pressed & SNES_PAD_B) {go(page==VIDEO || page==AUDIO || page==GAMEPLAY || page==CONTROLS || page==REWIND ? OPTIONS : page==LEVEL ? MODES : ROOT);return 0;}
    if(page==ACHIEVEMENTS) {
        if(pressed & SNES_PAD_UP)selected=(selected+n-1)%n;
        if(pressed & SNES_PAD_DOWN)selected=(selected+1)%n;
        if(pressed & SNES_PAD_LEFT)selected=selected>=6 ? selected-6 : 0;
        if(pressed & SNES_PAD_RIGHT)selected=selected+6<n ? selected+6 : n-1;
        return 0;
    }
    if(pressed & SNES_PAD_UP)selected=(selected+n-1)%n;
    if(pressed & SNES_PAD_DOWN)selected=(selected+1)%n;
    if(list && list[selected].key!=NONE) {
        if(pressed & (SNES_PAD_LEFT|SNES_PAD_RIGHT|SNES_PAD_A|SNES_PAD_START)) {
            const Item *it=&list[selected];int v=get(it->key)+((pressed & SNES_PAD_LEFT) ? -it->step : it->step);
            if(v>it->max)v=it->min;if(v<it->min)v=it->max;set(it->key,v);
        }
        return 0;
    }
    if(!(pressed & (SNES_PAD_A|SNES_PAD_START)))return 0;
    if(page==LEVEL) {if(selected==n-1)go(MODES);else {sciv_level_select_start(selected,0);visible=0;release=1;*freeze=0;}return 0;}
    int next=list[selected].page;
    if(next>=0) {if(next==ACHIEVEMENTS)sciv_achievements_menu_opened();go(next);}
    else if(next==-2) {visible=0;release=departing=1;*freeze=0;return SNES_PAD_START;}
    else if(next==-3) {visible=0;handoff=12;release=departing=1;*freeze=0;}
    else if(next==-4 || next==-5) {if(next==-4)sciv_boss_rush_start();else sciv_one_life_start();visible=0;release=1;*freeze=0;}
    else if(next==-6)*action=2;
    else if(next==-7) {if(page==AUDIO)sciv_open_audio_settings();*action=1;}
    return 0;
}
static int showing(void) {
    /* New-game state begins before the howl/title fade has finished. Keep
     * covering Play Select until its BG maps are replaced by name entry.
     * USA title BGSC: $02/$12/$5A; name entry uses $03/$13/$59. */
    int outgoing=departing && g_ppu && g_ppu->bgXsc[0]==0x02 &&
        g_ppu->bgXsc[1]==0x12 && g_ppu->bgXsc[2]==0x5a;
    return enabled && !sciv_other_castle_active() &&
        (outgoing || (word(0x32)==1 && (word(0x34)==1 || release || handoff)));
}
int sciv_title_overlay_scale(void) {return showing() && page==ACHIEVEMENTS ? 3 : sciv_achievement_overlay_scale();}
void sciv_title_overlay(uint8_t *dst,int pitch,int w,int h) {
    if(showing()) {
        if(page==ACHIEVEMENTS) {sciv_achievement_browser(dst,pitch,w,h,selected);return;}
        enum {W=512,H=188};uint32_t panel[W*H];
        for(int i=0;i<W*H;i++)panel[i]=0xff101018;
        snes_ovl_fill_rect(panel,W,H,0,0,W,2,0xff9d7952);
        const char *headers[]={"PLAY SELECT","ADDITIONAL MODES","LEVEL SELECT","ACHIEVEMENTS","QUIT GAME?","OPTIONS","VIDEO","AUDIO","GAMEPLAY","CONTROLS","REWIND / LATENCY","CHEATS / GAME GENIE"};
        const char *heading=headers[page];snes_ovl_draw_text(panel,W,H,(W-(int)strlen(heading)*16)/2,6,heading,0xffe1bd7b,2);
        {
            int n;const Item *list=items(&n);int first=selected>=6 ? selected-5 : 0;if(first+6>n)first=n>6 ? n-6 : 0;
            for(int row=0;row<6 && first+row<n;row++) {
                int i=first+row,y=30+row*22;const char *s=list ? list[i].label : i==n-1 ? "BACK" : sciv_level_select_name(i);
                if(i==selected)snes_ovl_fill_rect(panel,W,H,6,y-2,W-12,20,0xff413323);
                snes_ovl_draw_text(panel,W,H,14,y,i==selected ? ">" : " ",0xffe1bd7b,2);
                char clipped[32];snprintf(clipped,sizeof(clipped),"%.28s",s);
                int scale=2;
                snes_ovl_draw_text(panel,W,H,40,y+(scale==1 ? 4 : 0),clipped,i==selected ? 0xffffdf9b : 0xffc8c8bc,scale);
                if(list && list[i].key) {char value[32];value_label(list[i].key,value,sizeof(value));snes_ovl_draw_text(panel,W,H,W-20-(int)strlen(value)*16,y,value,0xffe1bd7b,2);}
            }
            if(n>6) {char position[24];snprintf(position,sizeof(position),"%d/%d",selected+1,n);snes_ovl_draw_text(panel,W,H,W-16-(int)strlen(position)*8,165,position,0xff9d7952,1);}
        }
        snes_ovl_draw_text(panel,W,H,16,168,"A CHOOSE  B BACK  L/R CHANGE",0xff9d7952,2);
        snes_ovl_blit_panel_rect(dst,pitch,w,h,panel,W,H,0,h*130/224,w,h*94/224);
    }
    sciv_achievement_overlay(dst,pitch,w,h);
}





