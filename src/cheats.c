#include "cheats.h"
#include "common_rtl.h"
#include <stdio.h>
static int flags[SCIV_CHEAT_COUNT], hearts_pending;
static unsigned saved_timer, saved_room;
static int timer_held;
static unsigned word(unsigned p) { return g_ram[p]|(g_ram[p+1]<<8); }
static void put(unsigned p,unsigned v) { g_ram[p]=v;g_ram[p+1]=v>>8; }
int sciv_cheat_get(int c) { return c>=0 && c<SCIV_CHEAT_COUNT ? flags[c] : 0; }
void sciv_cheats_reset(void) { timer_held=0;hearts_pending=1; }
void sciv_cheats_init(void) {
    FILE *f=fopen("cheats.cfg","r");
    if(f) { for(int i=0;i<SCIV_CHEAT_COUNT;i++) { int v=0;if(fscanf(f,"%d",&v)==1)flags[i]=v==1; } fclose(f); }
    sciv_cheats_reset();
}
void sciv_cheat_set(int c,int enabled) {
    if(c<0 || c>=SCIV_CHEAT_COUNT)return;
    flags[c]=!!enabled;
    FILE *f=fopen("cheats.cfg","w");
    if(f) { for(int i=0;i<SCIV_CHEAT_COUNT;i++)fprintf(f,"%d\n",flags[i]);fclose(f); }
}
void sciv_cheats_before_frame(void) {
    timer_held=0;
    if(word(0x32)!=4)return;
    /* Refill before the guest can consume its final life. */
    if(flags[SCIV_CHEAT_LIVES])put(0x7c,0x99);
    if(word(0x70)!=5)return;
    if(flags[SCIV_CHEAT_INVINCIBLE] && word(0x13f4))put(0xbc,2);
    if(flags[SCIV_CHEAT_TIMER]) {
        saved_timer=word(0x13f0);saved_room=word(0x86);timer_held=1;
        /* Prevent timeout side effects at 001 before restoring the display. */
        if(saved_timer==1)put(0x13f0,2);
    }
}
void sciv_cheats_after_frame(void) {
    int state=word(0x70), playing=word(0x32)==4 && state==5;
    /* Room transitions pass through state 3; new lives use initialization
     * states 0-2. Do not refill hearts for ordinary room changes. */
    if(word(0x32)!=4 || state<=2 || state==7 || state==12)hearts_pending=1;
    if(playing && hearts_pending) { if(flags[SCIV_CHEAT_HEARTS])put(0x13f2,0x98);hearts_pending=0; }
    if(timer_held && playing && word(0x86)==saved_room)put(0x13f0,saved_timer);
    timer_held=0;
}


