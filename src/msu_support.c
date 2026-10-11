#include "msu_support.h"
#include "sha256.h"
#include "cpu_state.h"
#include "common_rtl.h"
#include "snes/msu1.h"
#include "desktop/host_relaunch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int active;
static char pack_base[4096];
void sciv_msu_set_rom_path(const char *path) {
 if(!path)return;
#ifdef _WIN32
 if(!_fullpath(pack_base,path,sizeof(pack_base)))pack_base[0]=0;
#else
 if(!realpath(path,pack_base))pack_base[0]=0;
#endif
 char *dot=strrchr(pack_base,'.');if(dot)*dot=0;
}
int sciv_msu_active(void) {return active;}
void sciv_msu_select(int value) {active=!!value;}
int sciv_msu_matches(const char *path) {
 FILE *f=fopen(path,"rb"); if(!f)return 0;
 unsigned char *data=malloc(1048577); if(!data){fclose(f);return 0;}
 size_t n=fread(data,1,1048577,f);fclose(f);
 unsigned char digest[32];char hex[65];sha256_compute(data,n,digest);free(data);
 for(int i=0;i<32;i++)sprintf(hex+2*i,"%02x",digest[i]);
 return n==1048576 && !strcmp(hex,"49e8321030cc5d923a1b03cd8ff83843e0fd0a2445a7799b945ba736dd91be3d");
}
static bool trace_track(void *context,const char *base,uint16_t track,char *path,size_t cap) {
 (void)context;snprintf(path,cap,"%s-%u.pcm",base,track);
 FILE *f=fopen(path,"rb");int found=f!=NULL;if(f)fclose(f);
 fprintf(stderr,"[sciv-msu] track=%u file=%s found=%d\n",track,path,found);return found;
}
void sciv_msu_rom_loaded(const unsigned char *rom,size_t size) {
 (void)rom;(void)size;if(active){
#ifdef _WIN32
 _putenv_s("SNESRECOMP_MSU1",pack_base);
#else
 setenv("SNESRECOMP_MSU1",pack_base,1);
#endif
 if(getenv("SCIV_MSU_TRACE"))msu1_set_track_resolver(trace_track,NULL);cpu_select_interpreted_program();RtlSetSaveRoot("saves/msu1");}
}
int sciv_msu_launch(const char *path,char *error,size_t cap) {
 if(!sciv_msu_matches(path)){snprintf(error,cap,"Choose the supported USA MSU-1 ROM (SHA-256 49e83210...).");return 0;}
 char file[4120];snprintf(file,sizeof(file),"%s",path);char *dot=strrchr(file,'.');if(dot)*dot=0;strcat(file,"-1.pcm");
 FILE *f=fopen(file,"rb");if(!f){snprintf(error,cap,"Place matching PCM tracks beside the ROM with the same filename prefix.");return 0;}fclose(f);
 const char *args[]={"--msu-game","--no-launcher","--rom",path};
 if(!snesrecomp_host_relaunch(4,args)){snprintf(error,cap,"Could not restart the game.");return 0;}return 1;
}
