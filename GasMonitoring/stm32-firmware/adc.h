#ifndef __ADC_H
#define __ADC_H

/**
 * @brief ADC1 채널 6(PA6) 초기화 (12-bit Resolution, 단일 변환 모드)
 */
void ADC1_Init(void);

/**
 * @brief 가스 센서(PA6)의 12비트 ADC 변환 수치 읽기 (0 ~ 4095)
 * @return 12비트 디지털 변환 결과값
 */
unsigned short ADC1_Read(void);

#endif // __ADC_H