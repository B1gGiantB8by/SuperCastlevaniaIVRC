#define SDL_MAIN_HANDLED 1
#include "../src/cheats.c"
#include <assert.h>
uint8 g_ram[0x20000];
int main(void) {
 sciv_cheats_reset();for(int i=0;i<4;i++)flags[i]=1;
 put(0x32,4);put(0x70,3);sciv_cheats_after_frame();
 put(0x70,5);put(0x13f4,16);put(0x13f0,0x300);sciv_cheats_after_frame();assert(word(0x13f2)==0x98);
 put(0x13f2,0x97);sciv_cheats_before_frame();assert(word(0x7c)==0x99 && word(0xbc)==2);
 put(0x13f0,0x299);sciv_cheats_after_frame();assert(word(0x13f0)==0x300 && word(0x13f2)==0x97);
 put(0x70,3);sciv_cheats_after_frame();put(0x70,5);sciv_cheats_after_frame();assert(word(0x13f2)==0x97);
 put(0x70,12);sciv_cheats_after_frame();put(0x70,3);sciv_cheats_after_frame();put(0x70,5);sciv_cheats_after_frame();assert(word(0x13f2)==0x98);
 put(0x13f0,1);sciv_cheats_before_frame();assert(word(0x13f0)==2);put(0x13f0,1);sciv_cheats_after_frame();assert(word(0x13f0)==1);
 sciv_cheats_before_frame();put(0x86,1);put(0x13f0,0x500);sciv_cheats_after_frame();assert(word(0x13f0)==0x500);
 for(int i=0;i<4;i++)flags[i]=0;put(0xbc,0);put(0x7c,2);sciv_cheats_before_frame();assert(word(0xbc)==0 && word(0x7c)==2);
 puts("Cheat behavior checks passed");return 0;
}
