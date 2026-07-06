/*
 * buzzer.h
 *
 * Author: kccistc
 *
 * 부저(스피커) 제어 - Timer3 하드웨어 PWM(OC3A) 사용
 *  - PIN : PE3 (OC3A, ATmega128 하드웨어 고정 핀 - 변경 불가)
 *  - 방식: CTC 모드에서 비교일치 시 OC3A 출력을 토글 -> 사각파 생성
 *
 * 음이름 -> OCR3A 값 매핑 (참고 코드 Speaker.c 기준, 8분주 CTC)
 * OCR3A = 1,000,000 / 원하는 주파수(Hz)
 * 예) DO_01 = 1911 -> 약 523Hz(C5)
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define DO_01   1911
#define DO_01_H 1817
#define RE_01   1703
#define RE_01_H 1607
#define MI_01   1517
#define FA_01   1432
#define FA_01_H 1352
#define SO_01   1276
#define SO_01_H 1199
#define LA_01   1136
#define LA_01_H 1073
#define TI_01   1012
#define DO_02   956
#define DO_02_H 909
#define RE_02   851
#define RE_02_H 804
#define MI_02   758
#define FA_02   716
#define FA_02_H 676
#define SO_02   638
#define SO_02_H 602
#define LA_02   568
#define LA_02_H 536
#define TI_02   506
#define DO_03   478
#define DO_03_H 450
#define RE_03   425
#define RE_03_H 401
#define MI_03   378

void init_buzzer(void);
void buzzer_beep(void);                                   // 단순 비프음 (키패드 아닌 다른 입력용)
void buzzer_tone(uint16_t ocr3a_value, uint16_t duration_ms);  // OCR3A 값으로 직접 소리내기
void buzzer_beep_for_key(uint8_t key);                     // 키패드 키에 매핑된 음 재생 (S1~S16)

#endif /* BUZZER_H_ */