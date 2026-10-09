#include "other_castle.h"
#include "sha256.h"
#include "cpu_state.h"
#include "common_rtl.h"
#include "desktop/host_relaunch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int active;
static char paths[2][4096];
static const char *hack_hash = "8e920c40c3810d624f686d231ec884507a421e35e3e2ab4f541c3b9e596d3f40";
int sciv_other_castle_active(void) { return active; }
void sciv_other_castle_select(int value) { active = !!value; }
static int rom_matches(const char *path, size_t expected_size, const char *expected_hash) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char *rom = malloc(4194305);
    if (!rom) { fclose(f); return 0; }
    size_t n = fread(rom, 1, 4194305, f);
    fclose(f);
    unsigned char digest[32]; char hex[65];
    sha256_compute(rom, n, digest); free(rom);
    for (int i=0; i<32; ++i) sprintf(hex+2*i, "%02x", digest[i]);
    return n == expected_size && strcmp(hex, expected_hash) == 0;
}
int sciv_other_castle_matches(const char *path) { return rom_matches(path,4194304,hack_hash); }
const char *sciv_other_castle_path(int hack) { return paths[!!hack]; }
void sciv_other_castle_set_path(int hack, const char *path) {
    snprintf(paths[!!hack], sizeof(paths[0]), "%s", path);
    FILE *f = fopen("other-castle-roms.cfg", "w");
    if (f) { fprintf(f, "%s\n%s\n", paths[0], paths[1]); fclose(f); }
}
void sciv_other_castle_init(void) {
    FILE *f = fopen("other-castle-roms.cfg", "r");
    if (f) {
        for(int i=0;i<2;i++) if(fgets(paths[i],sizeof(paths[i]),f)) paths[i][strcspn(paths[i],"\r\n")]=0;
        fclose(f);
    }
    f=fopen("rom.cfg","r");
    if(f) {
        char cached[4096];
        if(fgets(cached,sizeof(cached),f)) {
            cached[strcspn(cached,"\r\n")]=0;
            int hack=sciv_other_castle_matches(cached);
            if(!paths[hack][0]) snprintf(paths[hack],sizeof(paths[hack]),"%s",cached);
        }
        fclose(f);
    }
    const char *names[2] = {"Super Castlevania IV (USA).sfc", "SuperCastlevaniaIVOtherCastle.sfc"};
    for(int i=0;i<2;i++) if(!paths[i][0]) {
        for(int depth=0;depth<3;depth++) {
            char candidate[4096];
            snprintf(candidate,sizeof(candidate),"%s%s",depth==0 ? "" : depth==1 ? "../" : "../../",names[i]);
            f=fopen(candidate,"rb");
            if(f) { fclose(f); snprintf(paths[i],sizeof(paths[i]),"%s",candidate); break; }
        }
    }
}
int sciv_other_castle_launch(int hack, char *error, size_t cap) {
    const char *path = paths[!!hack];
    FILE *f = fopen(path,"rb");
    if(!f) { snprintf(error,cap,"Choose your %s ROM first.",hack ? "patched Other Castle" : "stock USA"); return 0; }
    fclose(f);
    if(hack && !sciv_other_castle_matches(path)) {
        snprintf(error,cap,"This ROM does not match the supplied Other Castle Easy build (4 MiB, SHA-256 8e920c40...)."); return 0;
    }
    if(!hack && !rom_matches(path,1048576,"0ef6f4cce5a2273fa49fe1ce724e0048a8e39c91da6b00dbb693fe1ba909177d")) {
        snprintf(error,cap,"Choose the matching original USA ROM before returning to the original game."); return 0;
    }
    const char *args[] = {hack ? "--other-castle" : "--stock-game", "--no-launcher", "--rom", path};
    if(!snesrecomp_host_relaunch(4,args)) { snprintf(error,cap,"Could not restart the game."); return 0; }
    return 1;
}
void sciv_other_castle_rom_loaded(const unsigned char *rom, size_t size) {
    (void)rom; (void)size;
    if(active) {
        /* Changed guest code must never dispatch into stock-ROM AOT bodies. */
        cpu_select_interpreted_program();
        RtlSetSaveRoot("saves/other-castle");
    }
}
