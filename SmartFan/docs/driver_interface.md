# 드라이버 인터페이스

| 장치 | 호출 | 데이터 |
|---|---|---|
| `/dev/ultrasonic` | read | native int, 거리 mm |
| `/dev/fan_encoder` | read | native int steps + unsigned int pressed, 총 8 bytes |
| `/dev/fan_pwm` | write | native int, 0/2/5/8만 허용 |
| `/dev/fan_led` | write | native int, 단계 0/1/2/3 (LED 0/2/5/8개) |

같은 타깃 ABI로 빌드한다. State/event enum은 사용자 공간 전용이며 직접 전송하지 않는다.
encoder steps는 read 사이의 누적 회전량이고 pressed는 KEY LOW일 때 1이다.
네 가지 풍량은 OFF=0/S1=2/S2=5/S3=8이며 현재 driver duty는 0/60/80/100%다.
LED 드라이버에는 speed 대신 mode에 해당하는 0/1/2/3을 전달한다.
Main/Output은 모터에 speed, LED에 mode를 전달한다. 초기 상태와 종료 시 모두 OFF다.
LED 장치 열기/출력 실패는 로그를 남기고 LED 장치를 해제하여 모터 제어를 유지한다.
모터 출력 실패는 기존대로 Main의 정지/오류 종료 경로로 전달한다.
다른 값과 전송 크기는 EINVAL, 사용자 공간 short read/write는 EIO로 처리한다.

초음파 read는 측정 전 60ms 대기하고 Echo를 최대 50ms 기다린다.
ETIMEDOUT/EBUSY/유효하지 않은 거리는 자동 모드에서 정지 이벤트다.
그 외 장치 오류는 Main이 OFF 요청 후 실패 종료한다. SIGINT/SIGTERM도 OFF를 요청한다.
출력 장치와 엔코더는 각각 단일 open만 허용한다. 출력 close/release는 정지한다.
입력의 open/close는 Event, 출력의 open/close는 Output이 담당한다.

Event는 30ms 버튼 debounce, 짧은 release에서 POWER_TOGGLE, >=2초에 LONG_PRESS를
한 번 생성한다. 긴 누름 이후 release에는 POWER_TOGGLE을 생성하지 않는다.
접근/버튼/회전 이벤트를 대기열로 전달하며 두 칸 이상의 회전도 단계 끝까지 반영한다.
Main은 시작과 종료에 OFF를 요청하고, 변화가 있을 때만 출력/로그를 갱신한다.

`--dry-run`은 모터 장치를 열거나 출력 명령을 보내지 않는다. 입력 장치는 여전히 필요하다.
`fan_ioctl.h`의 GET_ECHO_US 매크로는 현재 드라이버에 구현되어 있지 않다.
저장소의 `fan_distance` 실행 파일은 소스/빌드 규칙이 없어 현재 통합 앱으로 사용하지 않는다.
