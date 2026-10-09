#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int sciv_other_castle_active(void);
void sciv_other_castle_select(int active);
int sciv_other_castle_matches(const char *path);
void sciv_other_castle_init(void);
const char *sciv_other_castle_path(int hack);
void sciv_other_castle_set_path(int hack, const char *path);
int sciv_other_castle_launch(int hack, char *error, size_t cap);
void sciv_other_castle_rom_loaded(const unsigned char *rom, size_t size);
#ifdef __cplusplus
}
#endif
