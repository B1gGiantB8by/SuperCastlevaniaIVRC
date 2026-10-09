#include "other_castle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int interpreted, launches;
static char root[96];
void cpu_select_interpreted_program(void) { interpreted++; }
void RtlSetSaveRoot(const char *value) { snprintf(root,sizeof(root),"%s",value); }
int snesrecomp_host_relaunch(int count, const char *const *args) {
    assert(count==4); assert(strcmp(args[0],"--other-castle")==0);
    assert(strcmp(args[2],"--rom")==0); launches++; return 1;
}
int main(int argc,char **argv) {
    assert(argc==3);
    assert(sciv_other_castle_matches(argv[1]));
    assert(!sciv_other_castle_matches(argv[2]));
    assert(!sciv_other_castle_matches("missing.sfc"));
    char error[256];
    sciv_other_castle_set_path(1,argv[2]);
    assert(!sciv_other_castle_launch(1,error,sizeof(error)) && !launches);
    sciv_other_castle_set_path(1,argv[1]);
    assert(sciv_other_castle_launch(1,error,sizeof(error)) && launches==1);
    sciv_other_castle_select(0); sciv_other_castle_rom_loaded(NULL,0);
    assert(!interpreted && !root[0]);
    sciv_other_castle_select(1); sciv_other_castle_rom_loaded(NULL,0);
    assert(interpreted==1 && strcmp(root,"saves/other-castle")==0);
    puts("Other Castle: ROM identity, launch arguments, interpreter dispatch and save isolation passed.");
}
