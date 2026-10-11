#include "rumble.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    ScivRumbleFrame before = {4, 5, 0, 16}, after = before;
    /* The cartridge changes state before the first whip-level increment. */
    after.state = 10;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_WHIP);
    before = after;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_NONE);
    before.state = 5;
    after = before; after.state = 10;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_WHIP);
    after = before; after.hp = 14;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_DAMAGE);
    after = before; after.room++;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_NONE);
    after.state = 10;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_NONE);
    after = before; after.mode = 1;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_NONE);
    after = before; after.state = 3;
    assert(sciv_rumble_event(before, after) == SCIV_RUMBLE_NONE);
    puts("Rumble pickup and damage regression checks passed");
    return 0;
}
