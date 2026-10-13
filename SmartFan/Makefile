CC ?= gcc
PYTHON ?= python3
CPPFLAGS += -Iinclude -Iuser
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic
BUILD_DIR := build
APP := $(BUILD_DIR)/smart-fan
USER_SOURCES := user/main.c user/event/event_handler.c user/state/fan_state.c user/output/fan_output.c
USER_HEADERS := $(wildcard include/*.h user/*/*.h)
KDIR ?= /lib/modules/$(shell uname -r)/build
TEST_NAMES := test_event test_state test_pwm test_led_driver test_main
TEST_BINS := $(addprefix $(BUILD_DIR)/,$(TEST_NAMES))

.PHONY: all user kernel test test-build clean
all: user

user: $(APP)

$(BUILD_DIR):
	mkdir -p $@

$(APP): $(USER_SOURCES) $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(USER_SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

kernel:
	$(MAKE) -C kernel/ultrasonic KDIR="$(KDIR)"
	$(MAKE) -C kernel/fan_pwm KDIR="$(KDIR)"
	$(MAKE) -C kernel/fan_led KDIR="$(KDIR)"

test-build: $(TEST_BINS)

$(BUILD_DIR)/test_event: tests/test_event.c user/event/event_handler.c $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_pwm: tests/test_pwm.c user/output/fan_output.c $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_led_driver: tests/test_led_driver.c kernel/fan_led/fan_led_drv.c $(wildcard tests/kernel_mock/linux/*.h) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Wno-unused-parameter -Itests/kernel_mock $< $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/test_state: tests/test_state.c user/state/fan_state.c $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< user/state/fan_state.c $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/main_test.o: user/main.c $(USER_HEADERS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Dmain=smart_fan_main -c $< -o $@

$(BUILD_DIR)/test_main: tests/test_main.c user/state/fan_state.c $(BUILD_DIR)/main_test.o $(USER_HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< user/state/fan_state.c $(BUILD_DIR)/main_test.o $(LDFLAGS) $(LDLIBS) -o $@

test: test-build
	$(PYTHON) tests/test_load_motor.py
	@for test in $(filter-out $(BUILD_DIR)/test_main,$(TEST_BINS)); do \
		"$$test"; status=$$?; \
		if [ $$status -ne 0 ]; then exit $$status; fi; \
	done
	@for scenario in normal encoder auto dry term init initial sensor write invalid stop; do \
		$(BUILD_DIR)/test_main "$$scenario" || exit $$?; \
	done

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C kernel/ultrasonic KDIR="$(KDIR)" clean
	$(MAKE) -C kernel/fan_pwm KDIR="$(KDIR)" clean
	$(MAKE) -C kernel/fan_led KDIR="$(KDIR)" clean
