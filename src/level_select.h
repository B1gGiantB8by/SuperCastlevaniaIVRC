#ifndef SCIV_LEVEL_SELECT_H
#define SCIV_LEVEL_SELECT_H
#ifdef __cplusplus
extern "C" {
#endif
int sciv_level_select_count(void);
const char *sciv_level_select_name(int index);
int sciv_level_select_has_transition(int index);
void sciv_level_select_start(int index, int transition);
void sciv_level_select_tick(void);
#ifdef __cplusplus
}
#endif
#endif
