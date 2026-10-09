#include "achievements.h"
#include "boss_rush.h"
#include "common_rtl.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <sys/stat.h>
#endif
static const ScivAchievement catalog[]={
#include "achievement_catalog.inc"
};
static unsigned long long earned[SCIV_ACHIEVEMENT_COUNT];
static int enabled=1, initialized, queue[SCIV_ACHIEVEMENT_COUNT], count;
static unsigned elapsed;
static char error[128];
/* Host-only attempt data is deliberately excluded from guest save states.
 * Loading/rewinding cannot repair a failed challenge or remove an unlock. */
static int campaign_ready, quest_hard;
static int tracked=-1, damaged, restricted, campaign, deathless, no_continue, rush;
static unsigned previous_active=1, previous_hp;
static unsigned weapon_hearts;
static unsigned health, previous_state, previous_room, swing_frames, swing_dirs;
static unsigned word(unsigned p) { return g_ram[p]|(g_ram[p+1]<<8); }
static const unsigned rooms[]={7,9,0x11,0x14,0x17,0x22,0x29,0x2d,0x3b,0x35,0x3f,0x40,0x41,0x43};
static const int normal[]={2,3,4,5,6,7,8,9,10,11,12,13,14,16};
const ScivAchievement *sciv_achievement(int i) { return i>=0 && i<SCIV_ACHIEVEMENT_COUNT ? &catalog[i] : NULL; }
static int save(void) {
#ifdef _WIN32
    _mkdir("saves");
#else
    mkdir("saves",0755);
#endif
    FILE *f=fopen("saves/achievements.local.tmp","w");
    if(!f) goto fail;
    int ok=fprintf(f,"SCIV_ACHIEVEMENTS 1 %d\n",enabled)>0;
    for(int i=0;i<SCIV_ACHIEVEMENT_COUNT;i++)
        if(fprintf(f,"%u %llu\n",catalog[i].id ? catalog[i].id : 1000000u,earned[i])<0) ok=0;
    if(fclose(f)!=0) ok=0;
    if(!ok) goto fail;
#ifdef _WIN32
    if(!MoveFileExA("saves/achievements.local.tmp","saves/achievements.local",MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) goto fail;
#else
    if(rename("saves/achievements.local.tmp","saves/achievements.local")) goto fail;
#endif
    error[0]=0; return 1;
fail:
    strcpy(error,"Could not save local achievements. Check that the saves folder is writable."); return 0;
}
void sciv_achievements_init(void) {
    if(initialized) return;
    initialized=1;
    FILE *f=fopen("saves/achievements.local","r");
    if(!f) return;
    char magic[32]; int version,on;
    if(fscanf(f,"%31s %d %d",magic,&version,&on)==3 && !strcmp(magic,"SCIV_ACHIEVEMENTS") && version==1 && (on==0 || on==1)) {
        enabled=on;
        unsigned id; unsigned long long stamp;
        while(fscanf(f,"%u %llu",&id,&stamp)==2)
            for(int i=0;i<SCIV_ACHIEVEMENT_COUNT;i++) if(id==(catalog[i].id ? catalog[i].id : 1000000u)) earned[i]=stamp;
    } else strcpy(error,"Unrecognized achievement save format; progress was not loaded.");
    fclose(f);
}
int sciv_achievements_enabled(void) { sciv_achievements_init(); return enabled; }
const char *sciv_achievements_error(void) { return error; }
int sciv_achievement_unlocked(int i) { sciv_achievements_init(); return i>=0 && i<SCIV_ACHIEVEMENT_COUNT && earned[i]!=0; }
unsigned sciv_achievements_score(void) { unsigned score=0; for(int i=0;i<SCIV_ACHIEVEMENT_COUNT;i++) if(sciv_achievement_unlocked(i)) score+=catalog[i].points; return score; }
void sciv_achievements_timeline_changed(void) { tracked=-1; campaign=deathless=no_continue=rush=campaign_ready=0; swing_frames=swing_dirs=0; previous_state=previous_room=~0u; previous_active=1; }
void sciv_achievements_menu_opened(void) { if(tracked>=0) restricted=1; }
int sciv_achievements_set_enabled(int on) {
    sciv_achievements_init(); int old=enabled; enabled=!!on;
    if(!save()) { enabled=old; return 0; }
    sciv_achievements_timeline_changed(); count=0; elapsed=0; return 1;
}
int sciv_achievements_reset(void) {
    sciv_achievements_init(); unsigned long long old[SCIV_ACHIEVEMENT_COUNT]; memcpy(old,earned,sizeof(old)); memset(earned,0,sizeof(earned));
    if(!save()) { memcpy(earned,old,sizeof(old)); return 0; }
    count=0; elapsed=0; sciv_achievements_timeline_changed(); return 1;
}
static void unlock(int i) {
    if(!enabled || earned[i] || (sciv_boss_rush_status()!=0 && i!=36)) return;
    earned[i]=(unsigned long long)time(NULL); if(!earned[i]) earned[i]=1;
    if(!save()) { earned[i]=0; return; }
    if(count<SCIV_ACHIEVEMENT_COUNT) queue[count++]=i;
}
int sciv_achievements_toast(void) { return enabled && count ? queue[0] : -1; }
void sciv_achievements_toast_elapsed(unsigned ms) {
    if(!count) { elapsed=0; return; }
    elapsed+=ms;
    if(elapsed>=6000) { memmove(queue,queue+1,(--count)*sizeof(queue[0])); elapsed=0; }
}
void sciv_achievements_rush_start(int first) { sciv_achievements_timeline_changed(); rush=enabled && first==0; }
void sciv_achievements_rush_complete(void) { if(rush) unlock(36); rush=0; }
void sciv_achievements_tick(void) {
    sciv_achievements_init(); if(!enabled) return;
    unsigned state=word(0x70),room=word(0x86),hp=word(0x13f4),mode=word(0x32);
    int in_rush=sciv_boss_rush_status()!=0;
    if(in_rush) {
        /* Boss Rush has its own completion award. Campaign, boss-clear and
         * challenge detectors must not accrue progress during any rush state. */
        tracked=-1; campaign=campaign_ready=deathless=no_continue=0;
        swing_frames=swing_dirs=0; previous_active=1; previous_hp=hp;
        if(state==7 || state==12 || (mode==4 && state==5 && !hp)) rush=0;
        return;
    }
    if(mode==4 && state==3 && room==0 && campaign_ready && !in_rush) {
        campaign=1; campaign_ready=0; deathless=no_continue=1; quest_hard=(word(0x88)&8)!=0;
    }
    if(mode==4 && state==5 && !in_rush) {
        if(word(0x13f2)==0x99) unlock(1); /* hearts are packed BCD */
        /* Hold Y and move the loose whip in multiple directions. */
        unsigned input=word(0x20);
        if(input&0x4000) { swing_frames++; swing_dirs|=(input>>8)&15; }
        else { swing_frames=swing_dirs=0; }
        if(swing_frames>=120 && (swing_dirs&(swing_dirs-1))) unlock(0);
        if(room==0x1e) unlock(17);
        if(room==0x36) unlock(18);
        /* The invisible stair below the Dracula approach: require entering
         * the hidden lower ledge, rather than merely holding full equipment. */
        if(room==0x42 && word(0x54a)<0x80 && word(0x54e)>0xc0 && hp==16 && word(0x92)==2 && word(0x90)==2) unlock(15);
    }
    if(state==7 || state==12 || (mode==4 && state==5 && !hp)) { deathless=0; tracked=-1; if(in_rush) rush=0; }
    if(state==12) no_continue=0;
    if(mode==1) { campaign=0; campaign_ready=1; }
    if(mode==4 && state==5 && word(0xb8) && tracked<0) {
        for(int i=0;i<14;i++) if(room==rooms[i]) { tracked=i; health=hp; weapon_hearts=word(0x13f2); damaged=previous_active!=0 || hp<previous_hp; restricted=previous_active!=0; break; }
    }
    if(tracked>=0) {
        if(hp<health) damaged=1;
        health=hp;
        if(word(0x28)&0x1000) restricted=1; /* Start pressed */
        if(word(0x13f2)<weapon_hearts) restricted=1; /* subweapon heart expenditure */
        weapon_hearts=word(0x13f2);
        /* Same guest completion signals as Boss Rush. A phase with zero
         * boss HP is insufficient while the boss-active flag remains set. */
        if(hp && ((!word(0xb8) && (!word(0x13f6) || room!=rooms[tracked])) || state==9 || mode==7)) {
            int boss=tracked; tracked=-1;
            unlock(normal[boss]);
            if(!damaged) unlock(19+boss);
            if(boss==13) {
                if(!damaged && !restricted) unlock(33);
                if(!in_rush && campaign) {
                    if(deathless) unlock(35);
                    if(no_continue && quest_hard) unlock(34);
                }
                campaign=0;
            }
        } else if(mode!=4 || (state!=5 && state!=9)) tracked=-1;
    }
    previous_room=room; previous_state=state; previous_active=word(0xb8)!=0; previous_hp=hp;
}
