#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
int sciv_msu_active(void);
int sciv_msu_matches(const char *path);
void sciv_msu_set_rom_path(const char *path);
void sciv_msu_select(int enabled);
void sciv_msu_rom_loaded(const unsigned char *rom, size_t size);
int sciv_msu_launch(const char *path, char *error, size_t cap);

#ifdef __cplusplus
}
#endif
