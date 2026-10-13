#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fan_common.h"
#include "event/event_handler.h"
#include "state/fan_state.h"
#include "output/fan_output.h"

static volatile sig_atomic_t stop_requested;

static void stop_handler(int signo)
{
    (void)signo;
    stop_requested = 1;
}

static void print_state(const fan_state_t *state)
{
    const char *name;

    switch (state->mode) {
    case FAN_MODE_S1:
        name = "S1";
        break;

    case FAN_MODE_S2:
        name = "S2";
        break;

    case FAN_MODE_S3:
        name = "S3";
        break;

    default:
        name = "OFF";
        break;
    }

    printf("팬 상태: %s / 상태 값: %d\n",
           name, state->speed);
    fflush(stdout);
}

int main(int argc, char **argv)
{
#ifndef _WIN32
    struct sigaction action = {0};
#endif
    bool automatic = false;
    bool dry_run = false;
    bool output_ready = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--auto") == 0) automatic = true;
        else if (strcmp(argv[i], "--dry-run") == 0) dry_run = true;
        else {
            fprintf(stderr, "Usage: %s [--auto] [--dry-run]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    const struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = 10000000L
    };

    fan_state_t state;
    int result = EXIT_FAILURE;

    stop_requested = 0;
#ifdef _WIN32
    if (signal(SIGINT, stop_handler) == SIG_ERR ||
        signal(SIGTERM, stop_handler) == SIG_ERR) {
#else
    action.sa_handler = stop_handler;

    if (sigemptyset(&action.sa_mask) < 0 ||
        sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
#endif
        perror("종료 신호 설정 실패");
        return EXIT_FAILURE;
    }

    fan_state_init(&state);

    if (event_handler_configure(automatic) < 0) {
        perror("입력 설정 실패");
        return EXIT_FAILURE;
    }

    if (!dry_run && fan_output_init() < 0) {
        perror("모터 장치 초기화 실패");
        fan_output_cleanup();
        event_handler_cleanup();
        return EXIT_FAILURE;
    }
    output_ready = !dry_run;

    if (output_ready && apply_fan_state(&state) < 0) {
        perror("초기 정지 출력 실패");
        goto shutdown;
    }

    if (automatic) puts("자동 모드: 20cm 이하 접근 시 S1, 멀어지면 OFF");
    else puts("엔코더 모드: 거리와 관계없이 조작");
    if (dry_run) puts("출력 확인 모드: 실제 모터 명령을 보내지 않습니다.");
    puts("엔코더 오른쪽: S1 → S2 → S3");
    puts("엔코더 왼쪽: S3 → S2 → S1");
    puts("버튼 짧게 눌렀다 떼기: ON/OFF / 2초 이상 누르기: OFF");
    puts("종료: Ctrl+C");

    print_state(&state);

    result = EXIT_SUCCESS;

    while (!stop_requested) {
        fan_event_t event;
        errno = 0;
        event = get_event();

        if (stop_requested)
            break;

        if (event == EVENT_ERROR) {
            if (errno) perror("초음파/엔코더 입력 실패");
            else fputs("입력 이벤트 오류\n", stderr);
            result = EXIT_FAILURE;
            break;
        }

        if (event < EVENT_NONE || event > EVENT_POWER_TOGGLE) {
            fprintf(stderr, "유효하지 않은 이벤트: %d\n",
                    (int)event);
            result = EXIT_FAILURE;
            break;
        }

        if (handle_event(&state, event)) {
            if (stop_requested)
                break;

            if (output_ready && apply_fan_state(&state) < 0) {
                perror("팬 PWM 출력 실패");
                result = EXIT_FAILURE;
                break;
            }
            print_state(&state);
        }
        if (nanosleep(&interval, NULL) < 0 &&
            errno != EINTR) {
            perror("대기 실패");
            result = EXIT_FAILURE;
            break;
        }
    }

shutdown:
    fan_state_init(&state);

    if (output_ready && apply_fan_state(&state) < 0) {
        perror("모터 정지 요청 실패");
        result = EXIT_FAILURE;
    }

    if (output_ready) fan_output_cleanup();
    event_handler_cleanup();

    puts("팬 제어 종료");
    return result;
}
