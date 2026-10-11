#ifndef SCIV_INTERPOLATION_H
#define SCIV_INTERPOLATION_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void sciv_interpolation_init(void);
void sciv_interpolation_reset(void);
void sciv_interpolation_set(int enabled);
int sciv_interpolation_enabled(void);
void sciv_interpolation_raster(void);
double sciv_interpolation_hz(double refresh);
int sciv_interpolation_draw(uint8_t *, size_t, const uint8_t *, int, int, double);
#ifdef __cplusplus
}
#endif
#endif

