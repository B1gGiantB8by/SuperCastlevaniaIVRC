/*
 * Super Castlevania IV — desktop host shim.
 *
 * Identity and hooks, nothing else. The host itself is the framework's
 * (snesrecomp/runner/src/desktop/host_main.h): the pre-boot launcher, ROM
 * resolution and digest checks, config.ini and keybinds.ini, the window and
 * the SDL / OpenGL presenters, audio, gamepads, the in-game save-state
 * browser and rewind filmstrip, the OSD, the pacing clock, crash handlers and
 * the post-mortem report, mod packages, Generate & rebuild, and the netplay
 * barrier when the project is built with it. A fix there reaches this
 * project on a submodule pull; nothing in this file needs to change for it.
 *
 * This file used to BE the host -- a ~700-line copy made at scaffold time --
 * so every project scaffolded before a fix kept the bug. Two ports of the
 * same game, a week apart, had different hosts.
 *
 * Nothing here ever ships a ROM. The player supplies one; the host makes
 * that easy (launcher, then positional argument, then a copy beside the
 * executable, then the rom.cfg cache, then a file picker) and checks what it
 * is handed against the digests in rom_identity.txt.
 *
 * Grow this file only with what is specific to THIS title: a custom
 * presenter (prepare_frame / draw_frame), an SPC player, a pacing rule
 * (keep_pacing_debt), a Mods provider. See SnesDesktopHostGame for the
 * complete list of hooks and what each is for.
 */

#include "host_main.h"
#include "game_rtl.h"
#include "game_timing.h"
#include "bezel.h"
#include "sciv_widescreen.h"
#include "boss_rush.h"
#include "level_select.h"
#include "achievements.h"
#include "achievement_overlay.h"
#include "achievement_sound.h"
#include "other_castle.h"
#include "title_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */

#ifndef __ANDROID__
#define SDL_MAIN_HANDLED 1
#endif
#include "desktop/sdl_compat.h"
#ifdef __ANDROID__
/* SDLActivity loads libmain.so and dlsyms its SDL_main. */
#define main SDL_main
#endif

#ifndef SNES_GAME_VERSION
#define SNES_GAME_VERSION "dev"
#endif

static void sciv_after_config(void) {
    sciv_other_castle_init();
    sciv_achievements_init();
    sciv_timing_init();
    sciv_bezel_init();
    sciv_widescreen_init();
}
static SnesDesktopHostGame kGameHost = {
    .on_rom_loaded       = sciv_other_castle_rom_loaded,
    .in_game_launcher   = 1,
    .display_aspect_supported = 1,
    .shader_supported   = 1,
    .compute_viewport    = sciv_bezel_viewport,
    .native_widescreen   = 1,
    .prepare_frame      = sciv_widescreen_prepare,
    .hold_rewind         = true,
    .display_name        = "Super Castlevania IV",
    .window_title        = "Super Castlevania IV",
    .region              = SNESRECOMP_ROM_REGION,
    .rom_file            = SNESRECOMP_ROM_FILE,
    .expected_sha256_hex = SNESRECOMP_ROM_EXPECTED_SHA256,
    .expected_crc32_hex  = SNESRECOMP_ROM_EXPECTED_CRC32,
    .game_id             = SNESRECOMP_ROM_GAME_ID,
    .build_version       = SNES_GAME_VERSION,
    .game_info           = &kGameInfo,
    .num_players         = 1,
    /* Battery-backed SRAM shows the launcher's SAVES panel. Leave NULL for a
     * title without one. The path is exe-relative. */
    .sram_path           = "saves/save.srm",
    .before_run_frame    = sciv_one_life_before_frame,
    .after_config        = sciv_after_config,
    .overlay_scale       = sciv_title_overlay_scale,
    .rewind_settings     = 1,
    .compose_overlay     = sciv_title_overlay,
    .filter_menu_input   = sciv_title_input,
    .on_shutdown         = sciv_achievement_sound_shutdown,
};

#ifndef __ANDROID__
#undef main   /* desktop: keep plain main() even if SDL_main.h remapped it */
#endif
int main(int argc, char **argv)
{
    int selected = -1;
    const char *rom_path = NULL;
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"--other-castle")==0 || strcmp(argv[i],"--stock-game")==0) {
            selected=strcmp(argv[i],"--other-castle")==0;
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i;
        } else if(strcmp(argv[i],"--rom")==0 && i+1<argc) rom_path=argv[++i];
        else if(argv[i][0]!='-' && (strstr(argv[i],".sfc") || strstr(argv[i],".smc"))) rom_path=argv[i];
    }
    char cached[4096];
    if(!rom_path && selected<0) {
        const char *base=SDL_GetBasePath();
        char cache_path[4096];
        snprintf(cache_path,sizeof(cache_path),"%srom.cfg",base ? base : "");
#if !SNESRECOMP_SDL3
        SDL_free((void *)base);
#endif
        FILE *f=fopen(cache_path,"r");
        if(f) { if(fgets(cached,sizeof(cached),f)) { cached[strcspn(cached,"\r\n")]=0; rom_path=cached; } fclose(f); }
    }
    if(selected<0) selected=rom_path && sciv_other_castle_matches(rom_path);
    sciv_other_castle_select(selected);
    if(selected) {
        kGameHost.display_name="Other Castle (Easy)";
        kGameHost.window_title="Super Castlevania IV — Other Castle";
        kGameHost.rom_file="SuperCastlevaniaIVOtherCastle.sfc";
        kGameHost.expected_sha256_hex="8e920c40c3810d624f686d231ec884507a421e35e3e2ab4f541c3b9e596d3f40";
        kGameHost.expected_crc32_hex=NULL;
        kGameHost.game_id="supercastlevaniaiv-other-castle-easy";
        kGameHost.sram_path="saves/other-castle/save.srm";
    }
    for(int i=1;i<argc;i++) {
        if(strcmp(argv[i],"--original-title-menu")==0) {
            sciv_title_enable(0);
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i; continue;
        }
        if(strcmp(argv[i],"--widescreen")==0 || strcmp(argv[i],"--no-widescreen")==0) {
            sciv_widescreen_override(strcmp(argv[i],"--widescreen")==0);
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i; continue;
        }
        if(strcmp(argv[i],"--enhanced-timing")==0 || strcmp(argv[i],"--original-timing")==0) {
            sciv_timing_override(strcmp(argv[i],"--enhanced-timing")==0);
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i; continue;
        }
        if(strncmp(argv[i],"--level-select=",15)==0 || strncmp(argv[i],"--level-map=",12)==0) {
            int map=strncmp(argv[i],"--level-map=",12)==0;
            char *end; long value=strtol(argv[i]+(map ? 12 : 15),&end,10);
            if(*end || value<1 || value>sciv_level_select_count() || (map && !sciv_level_select_has_transition((int)value-1))) {
                fprintf(stderr,"Level Select expects a valid 1-based stage entry; map entries must begin stages 2-B.\n"); return 1;
            }
            sciv_level_select_start((int)value-1,map);
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i; continue;
        }
        if(strcmp(argv[i],"--one-life")==0) {
            sciv_one_life_start();
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i; continue;
        }
        if(strcmp(argv[i],"--boss-rush")==0 || strncmp(argv[i],"--boss-rush=",12)==0) {
            int first=0;
            if(argv[i][11]=='=') {
                char *end; long value=strtol(argv[i]+12,&end,10);
                if(*end || value<1 || value>14) { fprintf(stderr,"--boss-rush expects a boss number from 1 to 14\n"); return 1; }
                first=(int)value-1;
            }
            sciv_boss_rush_start_at(first);
            for(int j=i;j<argc;j++) argv[j]=argv[j+1];
            --argc; --i;
        }
    }
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}
