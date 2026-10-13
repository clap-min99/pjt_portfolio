#include "fan_state.h"

static void set_mode(fan_state_t *state, fan_mode_t mode)
{
    state->mode = mode;
    state->power = mode != FAN_MODE_OFF;

    switch (mode) {
    case FAN_MODE_S1:
        state->speed = FAN_SPEED_S1;
        break;

    case FAN_MODE_S2:
        state->speed = FAN_SPEED_S2;
        break;

    case FAN_MODE_S3:
        state->speed = FAN_SPEED_S3;
        break;

    case FAN_MODE_OFF:
    default:
        state->speed = FAN_SPEED_MIN;
        break;
    }
}

void fan_state_init(fan_state_t *state)
{
    if (!state)
        return;

    set_mode(state, FAN_MODE_OFF);
}

bool handle_event(fan_state_t *state, fan_event_t event)
{
    fan_state_t previous;

    if (!state ||
        event < EVENT_NONE ||
        event > EVENT_POWER_TOGGLE ||
        event == EVENT_ERROR)
        return false;

    previous = *state;

    if (state->mode < FAN_MODE_OFF ||
        state->mode > FAN_MODE_S3) {
        fan_state_init(state);
    } else {
        switch (event) {
        case EVENT_NEAR:
            if (state->mode == FAN_MODE_OFF)
                set_mode(state, FAN_MODE_S1);
            break;

        case EVENT_FAR:
            set_mode(state, FAN_MODE_OFF);
            break;

        case EVENT_SENSOR_LOST:
            set_mode(state, FAN_MODE_OFF);
            break;

        case EVENT_LONG_PRESS:
            set_mode(state, FAN_MODE_OFF);
            break;

        case EVENT_POWER_TOGGLE:
            set_mode(state, state->mode == FAN_MODE_OFF ?
                     FAN_MODE_S1 : FAN_MODE_OFF);
            break;

        case EVENT_SHORT_PRESS:
            set_mode(state, state->mode == FAN_MODE_OFF ||
                     state->mode == FAN_MODE_S3 ? FAN_MODE_S1 :
                     (fan_mode_t)(state->mode + 1));
            break;

        case EVENT_SPEED_UP:
            if (state->mode == FAN_MODE_S1)
                set_mode(state, FAN_MODE_S2);
            else if (state->mode == FAN_MODE_S2)
                set_mode(state, FAN_MODE_S3);
            break;

        case EVENT_SPEED_DOWN:
            if (state->mode == FAN_MODE_S3)
                set_mode(state, FAN_MODE_S2);
            else if (state->mode == FAN_MODE_S2)
                set_mode(state, FAN_MODE_S1);
            break;

        case EVENT_NONE:
        default:
            break;
        }

        set_mode(state, state->mode);
    }

    /* 모터 출력이나 표시가 달라질 때만 true */
    return previous.mode != state->mode ||
           previous.power != state->power ||
           previous.speed != state->speed;
}
