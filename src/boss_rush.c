#include "boss_rush.h"
#include "level_select.h"
#include "achievements.h"
#include "common_rtl.h"
#include <string.h>
#include <stdio.h>

/* USA B64FFB12. Room/event and entrance formats are documented by SC4Ed:
 * https://github.com/bogaa/SC4Ed (SC4Core::LoadEvents, entrance expansion).
 * Reload and equipment fields: bogaa/Super-Castlevania-IV-Practice.
 * Only entrance DATA in the locally loaded ROM is temporarily changed.
 * Restore it when leaving the mode; never change the user's ROM file. */
typedef struct Arena { const char *name; unsigned room, x, y, cx, cy; } Arena;
static const Arena arenas[] = {
    {"Rowdain",       0x07,0x780,0xa5,0x700,0},
    {"Medusa",        0x09,0xe40,0x85,0xe00,0},
    {"Orphic Vipers", 0x11,0x0aa,0xbd,0x000,0},
    {"Puweyxil",      0x14,0x340,0x85,0x300,0},
    {"Koranot",       0x17,0x780,0x55,0x700,0},
    {"Dancing Specters",0x22,0x140,0x85,0x100,0},
    {"Grakul",        0x29,0x240,0x85,0x200,0},
    /* The left platform has fatal spike tiles at y=0x70..0x78. Enter in
     * the clear right-side gap, above (not inside) the floor at y=0xb0. */
    {"Frankenstein",  0x2d,0x8a0,0x85,0x800,0},
    {"Akmodan II",    0x35,0x240,0x85,0x200,0},
    {"Zapf Bat",      0x3b,0x240,0x55,0x200,0},
    {"Slogra",        0x3f,0x140,0x85,0x100,0},
    {"Gaibon",        0x40,0x140,0x85,0x100,0},
    {"Death",         0x41,0x240,0x85,0x200,0},
    {"Dracula",       0x43,0x040,0xa5,0x000,0},
};
/* The original game does not use this upper-WRAM tail. Keeping the entire
 * run here makes save states and rewind atomic with the guest machine and
 * leaves old states (no signature) as ordinary campaign states. */
enum { META=0x1ffe0, MAGIC=0x4252, STATUS=META+2, INDEX=META+4,
       SEEN=META+6, CLOCK=META+8, LOADING=META+12, GAME_OVER=META+14 };
static int command;
static int level_command, level_index, level_transition;
static unsigned first;
static unsigned char original[14][16];
static int patched;
static unsigned word(unsigned p) { return g_ram[p] | (g_ram[p+1]<<8); }
static void put(unsigned p,unsigned v) { g_ram[p]=(uint8)v; g_ram[p+1]=(uint8)(v>>8); }
static unsigned offset(unsigned addr) { return ((addr>>16)&127)*0x8000+(addr&0x7fff); }
static unsigned romword(unsigned addr) { const uint8 *p=g_rom+offset(addr); return p[0]|(p[1]<<8); }
static const unsigned tables[]={0x85be90,0x85be92,0x85cb92,0x85cb94,0x85cdb2,0x85cdb4,0x85be08,0x85c138};
static void entrances(int enable) {
    if(!g_rom || enable==patched) return;
    for(unsigned i=0;i<14;i++) {
        const Arena *a=&arenas[i];
        unsigned values[]={a->x,a->y,a->cx,a->cy,a->cx,a->cy,0,2};
        for(unsigned j=0;j<8;j++) {
            uint8 *p=(uint8 *)g_rom+offset(tables[j]+a->room*(j<6 ? 4 : 2));
            if(enable) { memcpy(&original[i][j*2],p,2); p[0]=(uint8)values[j]; p[1]=(uint8)(values[j]>>8); }
            else memcpy(p,&original[i][j*2],2);
        }
    }
    patched=enable;
}
void sciv_boss_rush_start_at(int index) { level_command=0; sciv_one_life_stop(); first=index>=0 && index<14 ? (unsigned)index : 0; command=1; }
void sciv_boss_rush_start(void) { sciv_boss_rush_start_at(0); }
void sciv_boss_rush_stop(void) { command=2; }
int sciv_boss_rush_status(void) { return word(META)==MAGIC ? (int)word(STATUS) : 0; }
int sciv_boss_rush_index(void) { unsigned i=word(INDEX); return i<14 ? (int)i : 0; }
unsigned sciv_boss_rush_frames(void) { return sciv_boss_rush_status() ? word(CLOCK)|(word(CLOCK+2)<<16) : 0; }
const char *sciv_boss_rush_name(int i) { return i>=0 && i<14 ? arenas[i].name : ""; }
void sciv_boss_rush_restore(void) { entrances(sciv_boss_rush_status()==1); }
/* Direct room entry can skip the scrolling trigger. Enqueue the original
 * ROM boss event exactly as the guest event reader does (80:D710), leaving
 * graphics, music, health, attacks and defeat entirely to the game. */
static void start_boss_event(void) {
    if(word(0xb8)) return;
    unsigned room=arenas[sciv_boss_rush_index()].room;
    for(unsigned slot=0x580;slot<0xec0;slot+=0x40)
        if(word(slot+0x10)==0x2a || (room==0x11 && word(slot+0x10)==0x48 && !word(slot+0x14))) return;
    unsigned ptr=0x850000+romword(0x80d81b+room*2);
    for(unsigned n=0;n<512;n++,ptr+=6) {
        unsigned x=romword(ptr),y=romword(ptr+2),id=romword(ptr+4);
        if(x==0xffff && y==0xffff) return;
        unsigned kind=id&255;
        if((x&3)==1 || (kind!=0x2a && !(room==0x11 && id==0x48))) continue;
        memset(g_ram+0x580,0,0x40);
        put(0x58a,x&0xffc); put(0x58e,y&0xffc);
        put(0x590,kind); put(0x594,id>>8);
        put(0x586,romword(0x81ac00+kind*2)+word(0x84));
        put(0x5ae,romword(0x81ad00+kind*2));
        put(0x5a8,romword(0x81ab00+kind*2)&255); put(0x5aa,romword(0x81ab01+kind*2)&255);
        unsigned mask=((x>>12)&15)|((y>>8)&240);
        if((x&3)==2) { put(0x5b0,mask); put(0x1500+mask,word(0x1500+mask)|1); }
        return;
    }
}
static void hold_entrance(void) {
    const Arena *a=&arenas[sciv_boss_rush_index()];
    put(0x54a,a->x); put(0x54e,a->y);
    /* A room can be entered while Simon is still recoiling. Re-enter the
     * guest's standing-spawn state with its normal collision height. */
    put(0x552,0); put(0x556,0); put(0x576,0x1b);
    /* mainGameState04 still executes gameplay during its fade. Keep the
     * staged entrance safe while collision/objects are being replaced. */
    put(0x13f4,16); put(0xbc,2); put(0x1f84,0);
    put(0x558,0); put(0x55a,0); put(0x55c,0); put(0x55e,0);
    put(0xa0,a->cx); put(0xa2,a->cx); put(0xa4,a->cy); put(0xa6,a->cy);
    put(0x1280,a->cx); put(0x128a,a->cx); put(0x1298,a->cy); put(0x12a2,a->cy);
}
static void load_arena_room(unsigned room, unsigned loading) {
    unsigned i=(unsigned)sciv_boss_rush_index();
    put(0x86,room); put(0x13d4,0); put(0x13c2,0);
    /* The guest only initializes the room's countdown when this new-stage
     * flag is set (86:B534). Skipping the campaign's stage-clear sequence
     * otherwise carries an expiring timer across every boss in the run. */
    put(0x13e8,1);
    /* Defeat mutes the SPC without necessarily invalidating the track cache.
     * Force the normal room loader to send a fresh music command. */
    put(0x13e2,0xffff); put(0x4a,0);
    put(0xb8,0); put(0x13f4,16); put(0x13f2,40);
    put(0x8e,4); put(0x90,2); put(0x92,2); put(0x7c,1);
    put(0x13fa,0); put(0x70,3); put(SEEN,0); put(LOADING,loading);
    /* Install the entrance BEFORE the first room-loader frame, not after it. */
    hold_entrance();
    fprintf(stderr,"[boss-rush] %u/14: %s%s\n",i+1,arenas[i].name,loading==3 ? " (loading inherited graphics)" : "");
}
static void load_arena(void) {
    unsigned i=(unsigned)sciv_boss_rush_index();
    /* Room 43 only loads the hall's first graphics bank and projectiles.
     * Dracula's cape/body background tiles come from room 42 (86:8B15).
     * Run that normal loader first, retaining its VRAM for the final room. */
    load_arena_room(i==13 ? 0x42 : arenas[i].room,i==13 ? 3 : 1);
}
void sciv_boss_rush_tick(void) {
    /* Wait for reset to finish before changing the game state. */
    if(command==1 && word(0x32)>=1) {
        command=0; sciv_achievements_rush_start((int)first); put(META,MAGIC); put(STATUS,1); put(INDEX,first);
        put(CLOCK,0); put(CLOCK+2,0); entrances(1);
        put(GAME_OVER,0);
        /* Normal new-game initialization supplies controls and guest globals. */
        put(0x32,4); put(0x70,0); put(0x72,4); put(LOADING,2);
        return;
    }
    if(command==2) {
        command=0; put(META,0); entrances(0);
        /* Return through the original title-screen initialization. */
        put(0x32,1); put(0x34,0); return;
    }
    sciv_boss_rush_restore();
    if(sciv_boss_rush_status()==3) {
        /* Wait for the guest's actual Continue selection, not just death.
         * These flags live in WRAM so saved game-over screens behave alike. */
        if(word(0x32)==4 && word(0x70)==7) put(GAME_OVER,1);
        if(word(GAME_OVER) && word(0x32)==4 && word(0x70)<=3) {
            sciv_boss_rush_start();
        } else if(word(0x32)!=4) {
            put(META,0); /* Choosing the title/password path exits the mode. */
        }
        return;
    }
    if(sciv_boss_rush_status()!=1) return;
    unsigned state=word(0x70);
    if(word(LOADING)==2) { if(state==3) load_arena(); return; }
    if(word(LOADING)) {
        if(state==3 || state==4) hold_entrance();
        if(state==5) {
            if(word(LOADING)==3) { load_arena_room(0x43,1); return; }
            hold_entrance(); put(0xbc,0); put(LOADING,0); start_boss_event();
        }
        return;
    }
    if(state==12 || state==7 || !word(0x13f4)) {
        fprintf(stderr,"[boss-rush] Failed: state=%u health=%u position=%04X,%04X player=%u time=%u fatal=%u invulnerable=%u\n",state,word(0x13f4),word(0x54a),word(0x54e),word(0x552),sciv_boss_rush_frames(),word(0x1f84),word(0xbc));
        put(STATUS,3); put(GAME_OVER,state==7); entrances(0); return;
    }
    unsigned clock=sciv_boss_rush_frames()+1; put(CLOCK,clock); put(CLOCK+2,clock>>16);
    if(word(0xb8)) put(SEEN,1);
    /* Require a real fight before accepting the guest's completion signal;
     * a zero HUD alone also occurs during loading and boss phase changes. */
    if(word(SEEN) && ((!word(0xb8) && (!word(0x13f6) || word(0x86)!=arenas[sciv_boss_rush_index()].room)) || state==9 || word(0x32)==7)) {
        unsigned i=word(INDEX)+1;
        if(i==14) { sciv_achievements_rush_complete(); put(STATUS,2); entrances(0); fprintf(stderr,"[boss-rush] Complete\n"); }
        else { put(INDEX,i); load_arena(); }
    }
}



/* Host-owned policy cannot be bypassed by restoring a campaign snapshot. */
static int one_life, one_life_command, one_life_game_over, one_life_loading;
int sciv_one_life_active(void) { return one_life; }
void sciv_one_life_start(void) {
    level_command=0;
    one_life=1; one_life_command=1; one_life_game_over=0; one_life_loading=1;
    command=0; put(META,0); entrances(0);
}

/* USA room starts from the original game / SCIV Practice stage table:
 * https://github.com/bogaa/Super-Castlevania-IV-Practice
 * Original map initialization: 80:9B98 and 85:FF40.
 * Keep loading with the guest: no synthetic entrances or ROM patches. */
typedef struct Stage { const char *name; unsigned room, stage; } Stage;
static const Stage stages[] = {
    {"Stage 1-1",0x00,1},{"Stage 1-2",0x02,1},{"Stage 1-3",0x05,1},
    {"Stage 2-1",0x08,2},{"Stage 2-2",0x09,2},{"Stage 2-3",0x0a,2},
    {"Stage 3-1",0x0c,3},{"Stage 3-2",0x0d,3},{"Stage 3-3",0x0e,3},
    {"Stage 4-1",0x12,4},{"Stage 4-2",0x15,4},{"Stage 4-3",0x16,4},{"Stage 4-4",0x17,4},
    {"Stage 5-1",0x18,5},{"Stage 5-2",0x19,5},
    {"Stage 6-1 (Dracula's Castle)",0x1a,6},{"Stage 6-2",0x1c,6},{"Stage 6-3",0x1f,6},
    {"Stage 7-1",0x23,7},{"Stage 7-2",0x26,7},
    {"Stage 8-1",0x2a,8},{"Stage 8-2",0x2c,8},
    {"Stage 9-1",0x2e,9},{"Stage 9-2",0x31,9},
    {"Stage A-1 (10-1)",0x37,10},{"Stage A-2 (10-2)",0x39,10},
    {"Stage B-1 (11-1)",0x3c,11},{"Stage B-2 (11-2)",0x3e,11},
    {"Stage B-3 (11-3)",0x3f,11},{"Stage B-4 (Dracula)",0x42,11},
};
int sciv_level_select_count(void) { return (int)(sizeof(stages)/sizeof(stages[0])); }
const char *sciv_level_select_name(int i) { return i>=0 && i<sciv_level_select_count() ? stages[i].name : ""; }
int sciv_level_select_has_transition(int i) {
    return i>0 && i<sciv_level_select_count() && stages[i].stage!=stages[i-1].stage;
}
void sciv_level_select_start(int i,int transition) {
    if(i<0 || i>=sciv_level_select_count() || (transition && !sciv_level_select_has_transition(i))) return;
    sciv_one_life_stop(); command=0; put(META,0); entrances(0);
    level_index=i; level_transition=transition!=0; level_command=1;
}
void sciv_level_select_tick(void) {
    if(!level_command) return;
    if(level_command==1) {
        if(word(0x32)<1) return;
        /* Run the real new-game initializer before selecting the room. */
        put(0x1e02,0); put(0x88,0); put(0x32,4); put(0x70,0); put(0x72,4);
        put(0x13d4,0); put(0x13e8,0); put(0x4a,0);
        level_command=2; return;
    }
    if(word(0x32)!=4 || word(0x70)!=3) return;
    const Stage *s=&stages[level_index];
    put(0x86,s->room); put(0x13d4,0); put(0x13c2,0); put(0x13e8,1);
    put(0x13e2,0xffff); put(0x13e4,0xffff); put(0x4a,0);
    put(0x13fa,0); put(0xb8,0); put(0x72,0);
    if(level_transition) {
        put(0x1c00,0); put(0x1c02,s->stage-2); put(0x70,11);
    }
    sciv_achievements_timeline_changed();
    fprintf(stderr,"[level-select] %s%s\n",s->name,level_transition ? " (map transition)" : "");
    level_command=0;
}
void sciv_one_life_stop(void) {
    one_life=0; one_life_command=0; one_life_game_over=0; one_life_loading=0;
}
void sciv_one_life_tick(void) {
    if(!one_life) return;
    if(one_life_command) {
        if(word(0x32)<1) return;
        one_life_command=0;
        /* Clear the campaign room/checkpoint before the guest initializer. */
        put(0x86,0); put(0x13d4,0); put(0x13c2,0); put(0x13e8,1);
        put(0x13f4,16); put(0x13f2,5);
        put(0x8e,0); put(0x90,0); put(0x92,0);
        /* Death can mute the SPC while its cached track still looks loaded. */
        put(0x13e2,0xffff); put(0x4a,0);
        put(0x32,4); put(0x70,0); put(0x72,4); put(0x7c,1);
        return;
    }
    if(one_life_loading) {
        if(word(0x32)==4) {
            /* New-game initialization supplies five lives. Limit them before
             * the first room loader runs, without loading the room twice. */
            if(word(0x7c)>1) put(0x7c,1);
            if(word(0x70)==5) one_life_loading=0;
        }
        return;
    }
    if(word(0x32)!=4) { sciv_one_life_stop(); return; }
    unsigned state=word(0x70);
    /* $7C is the BCD life count. Preserve zero after the guest consumes the
     * final life; $13C0 is a shared scene timer used by the drawbridge. */
    if(word(0x7c)>1) put(0x7c,1);
    if(state==7) one_life_game_over=1;
    else if(one_life_game_over && state<=3) {
        one_life_game_over=0; one_life_command=1; one_life_loading=1;
        /* Request a fresh machine at the next host frame boundary. The reset
         * must run outside the current frame and its CPU/APU bookkeeping. */
        sciv_one_life_reset_machine();
    }
}

