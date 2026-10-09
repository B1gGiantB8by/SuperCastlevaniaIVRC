#include "achievement_sound.h"
#include "desktop/sdl_compat.h"
#include "config.h"
#include <stdio.h>
static Uint8 *sound;
static Uint32 length;
static SDL_AudioSpec spec;
#if SNESRECOMP_SDL3
static SDL_AudioStream *stream;
#else
static SDL_AudioDeviceID device;
#endif
void sciv_achievement_sound_play(void) {
    if(!g_config.enable_audio || g_config.volume<=0) return;
    if(!sound && !SDL_LoadWAV("assets/castlevania/achievements/unlock.wav",&spec,&sound,&length)) {
        fprintf(stderr,"[achievements] Cannot load unlock sound: %s\n",SDL_GetError()); return;
    }
#if SNESRECOMP_SDL3
    /* SDL mixes this logical stream with the game's audio on the default
     * device and converts the supplied WAV format without blocking gameplay. */
    if(!stream) stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
    if(!stream) { fprintf(stderr,"[achievements] Cannot open unlock audio: %s\n",SDL_GetError()); return; }
    SDL_ClearAudioStream(stream);
    SDL_SetAudioStreamGain(stream,(float)g_config.volume/100.f);
    if(!SDL_PutAudioStreamData(stream,sound,(int)length) || !SDL_FlushAudioStream(stream) || !SDL_ResumeAudioStreamDevice(stream))
        fprintf(stderr,"[achievements] Cannot play unlock sound: %s\n",SDL_GetError());
#else
    if(!device) device=SDL_OpenAudioDevice(NULL,0,&spec,NULL,0);
    if(!device) { fprintf(stderr,"[achievements] Cannot open unlock audio: %s\n",SDL_GetError()); return; }
    Uint8 *mixed=SDL_malloc(length);
    if(!mixed) return;
    SDL_memset(mixed,spec.silence,length);
    SDL_MixAudioFormat(mixed,sound,spec.format,length,g_config.volume*SDL_MIX_MAXVOLUME/100);
    SDL_ClearQueuedAudio(device);
    if(SDL_QueueAudio(device,mixed,length)!=0) fprintf(stderr,"[achievements] Cannot queue unlock sound: %s\n",SDL_GetError());
    SDL_free(mixed); SDL_PauseAudioDevice(device,0);
#endif
}
void sciv_achievement_sound_shutdown(void) {
#if SNESRECOMP_SDL3
    if(stream) SDL_DestroyAudioStream(stream);
    stream=NULL;
    SDL_free(sound);
#else
    if(device) SDL_CloseAudioDevice(device);
    device=0;
    SDL_FreeWAV(sound);
#endif
    sound=NULL; length=0;
}
