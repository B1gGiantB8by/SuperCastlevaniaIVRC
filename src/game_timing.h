#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void sciv_timing_init(void);
int sciv_timing_enhanced(void);
void sciv_timing_set(int enhanced);
void sciv_timing_override(int enhanced);
#ifdef __cplusplus
}
#endif
