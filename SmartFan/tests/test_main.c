/* Member 2 integration test: replace the Event/Output modules with mocks.
 * Build on Linux (or with host GCC) from the repository root:
 *   cc -std=c11 -Wall -Wextra -Werror -Iinclude -Iuser \
 *      -Dmain=smart_fan_main -c user/main.c -o build/main_test.o
 *   cc -std=c11 -Wall -Wextra -Werror -Iinclude -Iuser \
 *      tests/test_main.c user/state/fan_state.c build/main_test.o \
 *      -o build/test_main
 * Run each scenario in a fresh process:
 *   for case in normal encoder auto dry term init initial sensor write invalid stop; do
 *       build/test_main "$case" || exit 1
 *   done
 */
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event/event_handler.h"
#include "output/fan_output.h"

int smart_fan_main(int argc, char **argv);

static const char *scenario;
static int init_calls;
static int cleanup_calls;
static int event_calls;
static int output_calls;
static int speeds[16];

int event_handler_configure(bool automatic)
{
    assert(automatic == (strcmp(scenario, "auto") == 0));
    return 0;
}

void event_handler_cleanup(void) {}

int fan_output_init(void)
{
    ++init_calls;
    if (strcmp(scenario, "init") == 0) {
        errno = ENOENT;
        return -1;
    }
    return 0;
}

void fan_output_cleanup(void)
{
    ++cleanup_calls;
}

int apply_fan_state(const fan_state_t *state)
{
    assert(output_calls < (int)(sizeof(speeds) / sizeof(speeds[0])));
    assert(state->speed >= 0 && state->speed <= 8);
    assert(state->power ? state->speed >= 1 : state->speed == 0);
    speeds[output_calls++] = state->speed;

    if ((strcmp(scenario, "initial") == 0 && output_calls == 1) ||
        (strcmp(scenario, "write") == 0 && output_calls == 2) ||
        (strcmp(scenario, "stop") == 0 && output_calls == 2)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

fan_event_t get_event(void)
{
    static const fan_event_t events[] = {
        EVENT_NONE, EVENT_SHORT_PRESS, EVENT_NONE,
        EVENT_SHORT_PRESS, EVENT_SHORT_PRESS, EVENT_SHORT_PRESS,
        EVENT_LONG_PRESS
    };

    ++event_calls;
    /* Also prevents a broken main loop from making the test hang. */
    assert(event_calls <= 16);
    if (strcmp(scenario, "encoder") == 0) {
        static const fan_event_t sequence[] = {
            EVENT_POWER_TOGGLE, EVENT_SPEED_UP, EVENT_SPEED_UP,
            EVENT_SPEED_DOWN, EVENT_SPEED_DOWN, EVENT_POWER_TOGGLE,
            EVENT_POWER_TOGGLE
        };
        if (event_calls <= 7) return sequence[event_calls - 1];
    } else if (strcmp(scenario, "auto") == 0) {
        static const fan_event_t sequence[] = {
            EVENT_NEAR, EVENT_SPEED_UP, EVENT_SPEED_UP, EVENT_SPEED_DOWN,
            EVENT_FAR, EVENT_NEAR, EVENT_SENSOR_LOST
        };
        if (event_calls <= 7) return sequence[event_calls - 1];
    }
    if (strcmp(scenario, "sensor") == 0) {
        errno = ETIMEDOUT;
        return EVENT_ERROR;
    }
    if (strcmp(scenario, "invalid") == 0)
        return (fan_event_t)100;
    if (strcmp(scenario, "write") == 0)
        return EVENT_SHORT_PRESS;
    if (strcmp(scenario, "stop") == 0 || strcmp(scenario, "term") == 0) {
        assert(raise(SIGTERM) == 0);
        return EVENT_NONE;
    }
    if (event_calls <= 7)
        return events[event_calls - 1];

    assert(raise(SIGINT) == 0);
    /* A pending event must not restart the motor after a stop request. */
    return EVENT_SHORT_PRESS;
}

int main(int argc, char **argv)
{
    int result;

    assert(argc == 2);
    scenario = argv[1];
    char *args[] = {"smart-fan", strcmp(scenario, "dry") == 0 ?
                                "--dry-run" : "--auto", NULL};
    result = smart_fan_main(strcmp(scenario, "auto") == 0 ||
                           strcmp(scenario, "dry") == 0 ? 2 : 1, args);
    if (strcmp(scenario, "dry") == 0) {
        assert(result == EXIT_SUCCESS && init_calls == 0 && cleanup_calls == 0);
        assert(output_calls == 0 && event_calls == 8);
        puts("PASS: main integration (dry; no motor access)");
        return 0;
    }
    assert(init_calls == 1);
    assert(cleanup_calls == 1);

    if (strcmp(scenario, "encoder") == 0 || strcmp(scenario, "auto") == 0) {
        const int encoder_expected[] = {0, 2, 5, 8, 5, 2, 0, 2, 0};
        const int auto_expected[] = {0, 2, 5, 8, 5, 0, 2, 0, 0};
        const int *expected = strcmp(scenario, "auto") == 0 ?
                              auto_expected : encoder_expected;
        assert(result == EXIT_SUCCESS && event_calls == 8 && output_calls == 9);
        for (int i = 0; i < output_calls; ++i) assert(speeds[i] == expected[i]);
    } else if (strcmp(scenario, "normal") == 0) {
        const int expected[] = {0, 2, 5, 8, 2, 0, 0};

        assert(result == EXIT_SUCCESS);
        assert(event_calls == 8);
        assert(output_calls == 7);
        for (int i = 0; i < output_calls; ++i)
            assert(speeds[i] == expected[i]);
    } else if (strcmp(scenario, "term") == 0) {
        assert(result == EXIT_SUCCESS);
        assert(output_calls == 2 && event_calls == 1);
    } else {
        assert(result == EXIT_FAILURE);
        if (strcmp(scenario, "init") == 0) {
            assert(output_calls == 0 && event_calls == 0);
        } else if (strcmp(scenario, "initial") == 0) {
            assert(output_calls == 2 && event_calls == 0);
        } else if (strcmp(scenario, "write") == 0) {
            assert(output_calls == 3 && event_calls == 1);
            assert(speeds[1] == 2);
        } else {
            assert(strcmp(scenario, "sensor") == 0 ||
                   strcmp(scenario, "invalid") == 0 ||
                   strcmp(scenario, "stop") == 0);
            assert(output_calls == 2 && event_calls == 1);
        }
    }

    if (output_calls > 0) {
        assert(speeds[0] == 0);
        assert(speeds[output_calls - 1] == 0);
    }
    printf("PASS: main integration (%s)\n", scenario);
    return 0;
}
