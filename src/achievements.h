#ifndef SCIV_ACHIEVEMENTS_H
#define SCIV_ACHIEVEMENTS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define SCIV_ACHIEVEMENT_COUNT 37
typedef struct ScivAchievement { unsigned id; const char *name, *description; unsigned points; const char *badge; } ScivAchievement;
const ScivAchievement *sciv_achievement(int index);
void sciv_achievements_init(void);
void sciv_achievements_tick(void);
void sciv_achievements_timeline_changed(void);
void sciv_achievements_menu_opened(void);
void sciv_achievements_rush_start(int first);
void sciv_achievements_rush_complete(void);
int sciv_achievements_enabled(void);
int sciv_achievements_set_enabled(int enabled);
int sciv_achievements_reset(void);
int sciv_achievement_unlocked(int index);
unsigned sciv_achievements_score(void);
const char *sciv_achievements_error(void);
int sciv_achievements_toast(void);
void sciv_achievements_toast_elapsed(unsigned milliseconds);
#ifdef __cplusplus
}
#endif
#endif
