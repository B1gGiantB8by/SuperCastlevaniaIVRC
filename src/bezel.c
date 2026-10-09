#include "bezel.h"
#include "sciv_widescreen.h"
#include <stdio.h>
static int enabled;
void sciv_bezel_init(void) {
    enabled = 0;
    FILE *f = fopen("bezel.cfg", "r");
    if (f) { if (fscanf(f, "%d", &enabled) != 1) enabled = 0; fclose(f); }
    enabled = enabled == 1;
}
int sciv_bezel_enabled(void) { return enabled && !sciv_widescreen_enabled(); }
void sciv_bezel_set(int value) {
    enabled = !!value;
    if (enabled && sciv_widescreen_enabled()) sciv_widescreen_set(0);
    FILE *f = fopen("bezel.cfg", "w");
    if (f) { fprintf(f, "%d\n", enabled); fclose(f); }
    else fprintf(stderr, "[bezel] Could not save bezel.cfg\n");
}
void sciv_bezel_viewport(int fw, int fh, int w, int h, SnesDisplayViewport *v) {
    (void)fw; (void)fh;
    if (sciv_bezel_enabled()) *v = SnesDisplayAspect_FitViewport(4.0 / 3.0, w, h);
}
