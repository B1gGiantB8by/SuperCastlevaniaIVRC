#pragma once
#include "display_aspect.h"
#ifdef __cplusplus
extern "C" {
#endif
void sciv_bezel_init(void);
int sciv_bezel_enabled(void);
void sciv_bezel_set(int enabled);
void sciv_bezel_viewport(int fw, int fh, int w, int h, SnesDisplayViewport *v);
#ifdef __cplusplus
}
#endif
