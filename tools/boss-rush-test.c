#include "boss_rush.h"
#include "level_select.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t g_ram[0x20000];
static uint8_t rom[0x100000], untouched[0x100000], snapshot[0x20000];
const uint8_t *g_rom=rom;
static int machine_reset_requests;
void sciv_one_life_reset_machine(void) {
    ++machine_reset_requests; /* host performs it outside the frame */
}
static void put(unsigned p,unsigned v) { g_ram[p]=(uint8_t)v; g_ram[p+1]=(uint8_t)(v>>8); }
static unsigned word(unsigned p) { return g_ram[p] | (g_ram[p+1]<<8); }
static void begin(int index) {
    sciv_boss_rush_start_at(index);
    put(0x32,1); sciv_boss_rush_tick();
    assert(word(0x70)==0 && word(0x72)==4);
    put(0x70,3); sciv_boss_rush_tick();
    assert(word(0x70)==3 && word(0x13f4)==16);
    assert(word(0x13e8)==1); /* guest must reset this arena's countdown */
    assert(word(0x13e2)==0xffff && word(0x4a)==0);
    assert(word(0x552)==0 && word(0xbc)==2);
    assert(word(0x8e)==4 && word(0x90)==2 && word(0x92)==2);
    unsigned t=sciv_boss_rush_frames();
    put(0x552,13); put(0x576,0); put(0x13f4,0); put(0x1f84,1);
    put(0x70,4); sciv_boss_rush_tick();
    assert(word(0x552)==0 && word(0x576)==0x1b);
    assert(word(0x13f4)==16 && word(0x1f84)==0 && word(0xbc)==2);
    assert(sciv_boss_rush_status()==1);
    assert(sciv_boss_rush_frames()==t);
    put(0x70,5); sciv_boss_rush_tick();
    if(index==13) {
        assert(word(0x86)==0x43 && word(0x70)==3);
        put(0x70,5); sciv_boss_rush_tick();
    }
    assert(word(0xbc)==0); /* real combat damage resumes immediately */
}
int main(void) {
    for(unsigned i=0;i<sizeof(rom);i++) rom[i]=(uint8_t)(i*37);
    memcpy(untouched,rom,sizeof(rom));
    sciv_boss_rush_start(); sciv_boss_rush_tick();
    assert(!sciv_boss_rush_status()); /* boot is not ready */
    begin(0);
    assert(memcmp(rom,untouched,sizeof(rom))!=0);
    put(0x13f6,0); sciv_boss_rush_tick();
    assert(sciv_boss_rush_index()==0); /* loading HUD is not a kill */
    put(0xb8,1); put(0x13f6,16); sciv_boss_rush_tick();
    memcpy(snapshot,g_ram,sizeof(snapshot));
    unsigned t=sciv_boss_rush_frames();
    put(0x13f6,0); sciv_boss_rush_tick();
    assert(sciv_boss_rush_index()==0); /* multi-phase boss still active */
    put(0xb8,0); sciv_boss_rush_tick();
    assert(sciv_boss_rush_index()==1 && word(0x86)==9);
    assert(word(0x13e8)==1);
    memcpy(g_ram,snapshot,sizeof(snapshot)); sciv_boss_rush_restore();
    assert(sciv_boss_rush_index()==0 && sciv_boss_rush_frames()==t);
    for(int i=0;i<14;i++) {
        assert(sciv_boss_rush_index()==i);
        put(0x70,5); sciv_boss_rush_tick();
        if(i==13) { assert(word(0x70)==3); put(0x70,5); sciv_boss_rush_tick(); }
        put(0xb8,1); put(0x13f6,16); sciv_boss_rush_tick();
        put(0xb8,0); put(0x13f6,0); sciv_boss_rush_tick();
    }
    assert(sciv_boss_rush_status()==2);
    assert(memcmp(rom,untouched,sizeof(rom))==0);
    begin(0); put(0x13f4,0); sciv_boss_rush_tick();
    assert(sciv_boss_rush_status()==3);
    assert(memcmp(rom,untouched,sizeof(rom))==0);
    put(0x70,7); sciv_boss_rush_tick();
    assert(sciv_boss_rush_status()==3); /* wait for a Continue selection */
    put(0x70,3); put(0x13f4,16); sciv_boss_rush_tick(); sciv_boss_rush_tick();
    assert(sciv_boss_rush_status()==1 && sciv_boss_rush_index()==0);
    assert(sciv_boss_rush_frames()==0 && word(0x70)==0);
    put(0x70,3); sciv_boss_rush_tick(); put(0x70,5); sciv_boss_rush_tick();
    put(0x70,7); sciv_boss_rush_tick();
    put(0x32,1); sciv_boss_rush_tick();
    assert(sciv_boss_rush_status()==0); /* title exits rather than restarting */
    memcpy(g_ram,snapshot,sizeof(snapshot)); sciv_boss_rush_restore();
    assert(memcmp(rom,untouched,sizeof(rom))!=0);
    memset(g_ram,0,sizeof(g_ram)); sciv_boss_rush_restore();
    assert(sciv_boss_rush_status()==0 && memcmp(rom,untouched,sizeof(rom))==0);
    begin(13); sciv_boss_rush_stop(); sciv_boss_rush_tick();
    assert(sciv_boss_rush_status()==0 && word(0x32)==1);
    assert(memcmp(rom,untouched,sizeof(rom))==0);
    sciv_one_life_start();
    assert(sciv_one_life_active());
    put(0x32,1); sciv_boss_rush_tick(); sciv_one_life_tick();
    assert(word(0x70)==0 && word(0x72)==4);
    put(0x70,3); put(0x7c,5); sciv_one_life_tick();
    assert(word(0x70)==3 && word(0x7c)==1);
    put(0x70,5); put(0x13e2,0); sciv_one_life_tick();
    assert(word(0x70)==5 && word(0x13e2)==0); /* one room load, keep music */
    put(0x13c0,0xff); put(0x7c,9); sciv_one_life_tick();
    assert(word(0x7c)==1); /* bonus lives cannot add another attempt */
    assert(word(0x13c0)==0xff); /* preserve the drawbridge scene timer */
    put(0x7c,0); sciv_one_life_tick();
    assert(word(0x7c)==0); /* do not revive the consumed final life */
    put(0x86,0x35); put(0x70,7); sciv_one_life_tick();
    assert(sciv_one_life_active() && word(0x70)==7);
    put(0x1500,0xffff); put(0x13c0,0xff);
    put(0x70,3); sciv_one_life_tick();
    assert(machine_reset_requests==1 && word(0x1500)==0xffff);
    memset(g_ram,0,sizeof(g_ram)); /* next host frame resets the machine */
    put(0x32,1); sciv_one_life_tick();
    assert(sciv_one_life_active() && word(0x70)==0 && word(0x72)==4);
    put(0x70,5); sciv_one_life_tick();
    assert(word(0x70)==5);
    put(0x32,1); sciv_one_life_tick();
    assert(!sciv_one_life_active());
    sciv_one_life_start(); sciv_boss_rush_start();
    assert(!sciv_one_life_active()); /* mutually exclusive modes */
    sciv_level_select_start(15,1); /* castle map cancels the queued rush */
    sciv_boss_rush_tick();
    assert(sciv_boss_rush_status()==0);
    sciv_level_select_tick();
    assert(word(0x70)==0 && word(0x72)==4);
    put(0x70,3); sciv_level_select_tick();
    assert(word(0x86)==0x1a && word(0x70)==11 && word(0x1c02)==4);
    assert(word(0x13d4)==0 && word(0x13e8)==1);
    sciv_level_select_start(29,0);
    sciv_one_life_start(); /* starting another mode cancels pending select */
    sciv_one_life_tick(); put(0x70,3); sciv_level_select_tick();
    assert(word(0x86)==0 && sciv_one_life_active());
    sciv_level_select_start(29,0);
    assert(!sciv_one_life_active());
    sciv_level_select_tick(); put(0x70,3); sciv_level_select_tick();
    assert(word(0x86)==0x42 && word(0x70)==3);
    sciv_level_select_start(-1,0); sciv_level_select_start(30,0);
    sciv_level_select_start(1,1); sciv_level_select_tick();
    assert(word(0x86)==0x42 && word(0x70)==3);
    puts("Level Select mode switching, castle map, Dracula entry and invalid selections passed.");
    puts("One Life start, Continue, bonus lives, exit and mode switching passed.");
    puts("Boss Rush progression, phases, failure, rewind/load and ROM restoration passed.");
    return 0;
}
