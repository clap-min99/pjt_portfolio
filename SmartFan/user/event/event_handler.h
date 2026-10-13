#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "fan_common.h"

fan_event_t get_event(void);
/* Configure before the first get_event(); default is encoder only. */
int event_handler_configure(bool automatic);
void event_handler_cleanup(void);

#endif /* EVENT_HANDLER_H */
