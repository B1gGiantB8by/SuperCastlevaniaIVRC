#ifndef SCIV_BOSS_RUSH_H
#define SCIV_BOSS_RUSH_H
#ifdef __cplusplus
extern "C" {
#endif
void sciv_one_life_start(void);
void sciv_one_life_stop(void);
int sciv_one_life_active(void);
void sciv_one_life_tick(void);
void sciv_one_life_reset_machine(void);
void sciv_one_life_before_frame(void);
void sciv_boss_rush_start(void);
void sciv_boss_rush_start_at(int index);
void sciv_boss_rush_stop(void);
int sciv_boss_rush_status(void); /* 0 idle, 1 running, 2 complete, 3 failed */
int sciv_boss_rush_index(void);
unsigned sciv_boss_rush_frames(void);
const char *sciv_boss_rush_name(int index);
void sciv_boss_rush_tick(void);
void sciv_boss_rush_restore(void);
#ifdef __cplusplus
}
#endif
#endif
