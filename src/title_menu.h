#ifndef SCIV_TITLE_MENU_H
#define SCIV_TITLE_MENU_H
#include <stdint.h>
uint32_t sciv_title_input(uint32_t input, int *action, int *freeze);
void sciv_title_overlay(uint8_t *dst,int pitch,int width,int height);
void sciv_title_reset(void);
void sciv_title_enable(int enabled);
int sciv_title_overlay_scale(void);
#endif
