#ifndef SCIV_CHEATS_H
#define SCIV_CHEATS_H
#ifdef __cplusplus
extern "C" {
#endif
enum { SCIV_CHEAT_LIVES, SCIV_CHEAT_INVINCIBLE, SCIV_CHEAT_HEARTS, SCIV_CHEAT_TIMER, SCIV_CHEAT_COUNT };
int sciv_cheat_get(int cheat);
void sciv_cheat_set(int cheat,int enabled);
void sciv_cheats_init(void);
void sciv_cheats_reset(void);
void sciv_cheats_before_frame(void);
void sciv_cheats_after_frame(void);
#ifdef __cplusplus
}
#endif
#endif
