#ifndef FAN_OUTPUT_H
#define FAN_OUTPUT_H

#include "fan_common.h"

/* Return 0 on success, -1 on failure with errno set. */
int fan_output_init(void);
int apply_fan_state(const fan_state_t *state);
void fan_output_cleanup(void);

#endif /* FAN_OUTPUT_H */
