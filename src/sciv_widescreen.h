#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void sciv_widescreen_init(void);
int sciv_widescreen_enabled(void);
void sciv_widescreen_set(int enabled);
int sciv_widescreen_aspect(void);
void sciv_widescreen_set_aspect(int aspect);
void sciv_widescreen_override(int enabled);
void sciv_widescreen_prepare(int dw, int dh, int *width, int *height);
void sciv_widescreen_draw_policy(void);
#ifdef __cplusplus
}
#endif
