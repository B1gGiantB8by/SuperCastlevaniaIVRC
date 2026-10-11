#ifndef SCIV_RUMBLE_H
#define SCIV_RUMBLE_H

typedef struct {
    unsigned mode, state, room, hp;
} ScivRumbleFrame;

enum { SCIV_RUMBLE_NONE, SCIV_RUMBLE_DAMAGE, SCIV_RUMBLE_WHIP };

static inline int sciv_rumble_event(ScivRumbleFrame before, ScivRumbleFrame after) {
    if (before.mode != 4 || after.mode != 4 || before.state != 5 ||
        before.room != after.room) return SCIV_RUMBLE_NONE;
    if (after.state == 5 && before.hp <= 16 && after.hp < before.hp)
        return SCIV_RUMBLE_DAMAGE;
    /* USA $00DF00: the whip pickup sets state $0A before incrementing $92.
     * State $0A runs the upgrade animation, including the length upgrade.
     * The transition also detects pickups when the whip is already maxed. */
    if (after.state == 10) return SCIV_RUMBLE_WHIP;
    return SCIV_RUMBLE_NONE;
}

#endif
