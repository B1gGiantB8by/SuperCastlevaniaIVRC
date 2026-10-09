#include "achievements.h"
#include "common_rtl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8 g_ram[0x20000];
static int rush_status;
int sciv_boss_rush_status(void) { return rush_status; }
static void put(unsigned p,unsigned value) { g_ram[p]=value; g_ram[p+1]=value>>8; }
static void tick(void) { sciv_achievements_tick(); }
static void room(unsigned r) { put(0x32,4); put(0x70,5); put(0x86,r); put(0x13f4,16); }
static void begin(unsigned r) { room(r); put(0xb8,0); put(0x13f6,16); tick(); put(0xb8,1); tick(); }
static void defeat(void) { put(0x13f6,0); put(0xb8,0); tick(); }
int main(int argc,char **argv) {
    sciv_achievements_init();
    if(argc>1 && !strcmp(argv[1],"failure")) {
        assert(sciv_achievements_enabled());
        assert(!sciv_achievements_set_enabled(0)); assert(sciv_achievements_enabled());
        room(0); put(0x13f2,0x99); tick(); assert(!sciv_achievement_unlocked(1));
        assert(sciv_achievements_error()[0]); assert(sciv_achievements_toast()==-1);
        puts("Save failures preserve settings and do not falsely report an unlock."); return 0;
    }
    if(argc>1 && !strcmp(argv[1],"reload")) {
        assert(!sciv_achievements_enabled());
        assert(sciv_achievement_unlocked(1)); assert(sciv_achievement_unlocked(36));
        assert(sciv_achievements_score()==30); puts("Persistence across process restart passed."); return 0;
    }
    assert(sciv_achievements_set_enabled(1)); assert(sciv_achievements_reset());
    unsigned total=0; for(int i=0;i<37;i++) { assert(sciv_achievement(i)); total+=sciv_achievement(i)->points; }
    assert(total==593);
    const unsigned arenas[]={7,9,0x11,0x14,0x17,0x22,0x29,0x2d,0x3b,0x35,0x3f,0x40,0x41,0x43};
    const int boss_awards[]={2,3,4,5,6,7,8,9,10,11,12,13,14,16};
    for(int boss=0;boss<14;boss++) {
        begin(arenas[boss]); defeat();
        assert(sciv_achievement_unlocked(boss_awards[boss])); assert(sciv_achievement_unlocked(19+boss));
    }
    assert(sciv_achievements_reset());
    room(0); put(0x13f2,0x99); tick(); assert(sciv_achievement_unlocked(1));
    assert(sciv_achievements_toast()==1); sciv_achievements_toast_elapsed(6000); assert(sciv_achievements_toast()==-1);
    tick(); assert(sciv_achievements_toast()==-1); /* already earned */
    assert(sciv_achievements_set_enabled(0)); begin(7); defeat(); assert(!sciv_achievement_unlocked(2));
    assert(sciv_achievement_unlocked(1)); assert(sciv_achievements_set_enabled(1));
    begin(7); put(0x13f6,0); tick(); assert(!sciv_achievement_unlocked(2)); /* phase still active */
    defeat(); assert(sciv_achievement_unlocked(2)); assert(sciv_achievement_unlocked(19));
    begin(9); put(0x13f4,15); tick(); put(0x13f4,16); tick(); defeat();
    assert(sciv_achievement_unlocked(3)); assert(!sciv_achievement_unlocked(20)); /* healing cannot repair damage */
    begin(0x11); sciv_achievements_timeline_changed(); tick(); defeat();
    assert(sciv_achievement_unlocked(4)); assert(!sciv_achievement_unlocked(21)); /* loaded mid-fight */
    begin(0x43); sciv_achievements_menu_opened(); defeat();
    assert(sciv_achievement_unlocked(16)); assert(sciv_achievement_unlocked(32)); assert(!sciv_achievement_unlocked(33));
    assert(sciv_achievements_reset()); begin(0x43); put(0x13f2,0x98); tick(); defeat(); assert(!sciv_achievement_unlocked(33));
    assert(sciv_achievements_reset()); begin(0x43); defeat(); assert(sciv_achievement_unlocked(33));
    assert(!sciv_achievement_unlocked(35)); /* entering Dracula alone isn't a full quest */
    assert(sciv_achievements_reset());
    /* A genuine Stage 1 start arms full-quest challenges. Loading a late
     * campaign state and merely beating Dracula must never qualify. */
    put(0x32,1); tick(); put(0x32,4); put(0x70,3); put(0x86,0); put(0x88,8); tick();
    begin(0x43); defeat(); assert(sciv_achievement_unlocked(34)); assert(sciv_achievement_unlocked(35));
    assert(sciv_achievements_reset()); put(0x32,1); tick(); put(0x32,4); put(0x70,3); put(0x86,0); tick();
    put(0x70,7); tick(); begin(0x43); defeat(); assert(!sciv_achievement_unlocked(35)); assert(sciv_achievement_unlocked(34));
    assert(sciv_achievements_reset()); put(0x32,1); tick(); put(0x32,4); put(0x70,3); put(0x86,0); tick();
    put(0x70,12); tick(); begin(0x43); defeat(); assert(!sciv_achievement_unlocked(34));
    assert(sciv_achievements_reset()); sciv_achievements_rush_start(0); rush_status=1;
    for(int boss=0;boss<14;boss++) { begin(arenas[boss]); defeat(); }
    room(0x42); put(0x13f2,0x99); put(0x54a,0x40); put(0x54e,0xd0); put(0x92,2); put(0x90,2); tick();
    rush_status=3; room(7); begin(7); defeat();
    rush_status=2; room(0); tick();
    for(int i=0;i<37;i++) assert(!sciv_achievement_unlocked(i));
    rush_status=1; sciv_achievements_rush_start(0); sciv_achievements_rush_complete();
    for(int i=0;i<37;i++) assert(sciv_achievement_unlocked(i)==(i==36));
    rush_status=0;
    assert(sciv_achievements_reset()); sciv_achievements_rush_start(13); sciv_achievements_rush_complete(); assert(!sciv_achievement_unlocked(36));
    sciv_achievements_rush_start(0); sciv_achievements_timeline_changed(); sciv_achievements_rush_complete(); assert(!sciv_achievement_unlocked(36));
    sciv_achievements_rush_start(0); rush_status=1; room(7); put(0x13f4,0); tick(); sciv_achievements_rush_complete(); assert(!sciv_achievement_unlocked(36));
    rush_status=0; sciv_achievements_rush_start(0); sciv_achievements_rush_complete(); assert(sciv_achievement_unlocked(36));
    room(0); put(0x13f2,0x99); tick(); assert(sciv_achievement_unlocked(1));
    assert(sciv_achievements_set_enabled(0)); assert(sciv_achievements_score()==30);
    puts("Catalog, BCD hearts, boss phases, damage/healing, toggles, reset, state loads and Boss Rush eligibility passed.");
    return 0;
}
