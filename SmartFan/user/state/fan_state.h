#ifndef FAN_STATE_H
#define FAN_STATE_H

#include "fan_common.h"

/* Initialize OFF / power false / speed 0. NULL is ignored. */
void fan_state_init(fan_state_t *state);

/* SHORT: OFF -> S1 -> S2 -> S3 -> S1. LONG: any mode -> OFF.
 * POWER_TOGGLE: OFF <-> S1 (turning ON always starts S1).
 * SPEED_UP/DOWN: S1 <-> S2 <-> S3, saturated; ignored in OFF.
 * NEAR: start S1 only from OFF. FAR/SENSOR_LOST: OFF.
 * mode is authoritative; power/speed are derived as 0, 2, 5, 8.
 * Return true when mode or derived output fields change.
 * Valid events repair inconsistent output fields; invalid mode resets OFF.
 * NULL, EVENT_ERROR and unknown events are ignored; main handles errors.
 */
bool handle_event(fan_state_t *state, fan_event_t event);

#endif /* FAN_STATE_H */
