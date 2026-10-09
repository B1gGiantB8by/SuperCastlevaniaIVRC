#ifndef SCIV_ACHIEVEMENT_OVERLAY_H
#define SCIV_ACHIEVEMENT_OVERLAY_H
#include <stdint.h>
int sciv_achievement_overlay_scale(void);
void sciv_achievement_overlay(uint8_t *dst,int pitch,int width,int height);
void sciv_achievement_browser(uint8_t *dst,int pitch,int width,int height,int selected);
#endif
