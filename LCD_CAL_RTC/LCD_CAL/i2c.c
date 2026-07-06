/*
 * i2c.c
 *
 * Created: 2026-07-01 오후 2:23:44
 *  Author: kccistc
 */ 

#include "i2c.h"
#include "ds1307.h"   

void i2c_init(void);
uint8_t i2c_start(void);
uint8_t i2c_stop(void);
uint8_t i2c_slave_addr_send(uint8_t addr_rw); //I2C_transmit
uint8_t i2c_data_write(uint8_t data);
uint8_t i2c_data_read_acksend(void);
uint8_t i2c_data_read_nacksend(void);

void i2c_init(void)
{
	// 출력으로 설정하지 않는다.
	DS1307_DDR &= ~((1<<DS1307_SCL) | (1<<DS1307_SDA));
	
	TWSR = 0x00;
	TWBR = TWBR_VALUE;	// SCL 주파수 결정 register 100KHz
}


uint8_t i2c_start(void)
{
	uint8_t st;
	
	TWCR =  1 << TWINT | 1 << TWSTA | 1 << TWEN;
	// TWINT			TWSTA: START 요청 , TWEN: I2C HW 활성화
	// I2C HW가 동작이 완료될때까지 기다린다. 동작이 완료되면 TWINT flag를 1로 세트
	while (!(TWCR & (1<< TWINT)));
	
	st = TWSR & 0xf8;		// 7~3 bit 까지 값을 취함
	return (st == TWSR_START|| st == TWSR_REP_START) ? 0 : st;		// 성공이면 0, 아니면 error 상태 코드 반환
}

uint8_t i2c_stop(void)
{
	TWCR =  1 << TWINT | 1 << TWSTO | 1 << TWEN;
	// TWINT			TWSTA: STOP 요청 , TWEN: I2C HW 활성화
	// I2C HW가 동작이 완료될때까지 기다린다. 동작이 완료되면 TWINT flag를 1로 세트
	// STOP 후 TWINT clear 되기까지 대기
	_delay_us(10);

	return 0;   // [수정] 반환값이 없어서 정의되지 않은 값이 return 되던 것을 0으로 명시
}

//SLAVE_ADDR+R SLAVE_ADDR+W
uint8_t i2c_slave_addr_send(uint8_t addr_rw)
{
	uint8_t st;
	
	TWDR = addr_rw;
	TWCR =  1 << TWINT | 1 << TWEN;
	// TWINT			 TWEN: I2C HW 활성화
	// I2C HW가 동작이 완료될때까지 기다린다. 동작이 완료되면 TWINT flag를 1로 세트
	while (!(TWCR & (1<< TWINT)));
	st = TWSR & 0xf8;		// 7~3 bit 까지 값을 취함
	
	return (st);
}
//I2C_transmit
uint8_t i2c_data_write(uint8_t data)
{
	
	TWDR = data;
	TWCR =  1 << TWINT | 1 << TWEN;
	// TWINT			 TWEN: I2C HW 활성화
	// I2C HW가 동작이 완료될때까지 기다린다. 동작이 완료되면 TWINT flag를 1로 세트
	while (!(TWCR & (1<< TWINT)));
	
	return (TWSR & 0xf8);		// 7~3 bit 까지 값을 취함
}

uint8_t i2c_data_read_acksend(void)
{
	
	TWCR = 1 << TWINT | 1 << TWEN | 1 << TWEA ;		// TWEA = 1: ACK 자동 전송
	// TWINT			 TWEN: I2C HW 활성화
	// I2C HW가 동작이 완료될때까지 기다린다. 동작이 완료되면 TWINT flag를 1로 세트
	while (!(TWCR & (1<< TWINT)));
	
	return TWDR;		// 7~3 bit 까지 값을 취함
}


uint8_t i2c_data_read_nacksend(void)
{
	
	TWCR = 1 << TWINT | 1 << TWEN ;		// TWEA = 0: NACK 자동 전송
	
	while (!(TWCR & (1<< TWINT)));
	
	return TWDR;		// 7~3 bit 까지 값을 취함
}