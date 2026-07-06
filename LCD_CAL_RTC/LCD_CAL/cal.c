/*
 * cal.c
 *
 * Created: 2026-06-29 오후 4:08:20
 *  Author: kccistc
 */ 
#include "cal.h"
#include "keypad.h"
#include "lcd.h"

static void lcd_show_expr(t_cal *c);
static void lcd_show_result(int32_t result);
extern void lcd_write_data(uint8_t data);
extern void lcd_write_string(char *string);
extern void lcd_goto_xy(uint8_t row, uint8_t col);
extern void lcd_clear(void);
extern void init_keypad();
extern uint8_t keypad_scan(void);
static uint8_t check_parens(char *expr);

void cal_main(void);
void cal_input(t_cal *c, uint8_t key);
void cal_reset(t_cal *c);
void cal_redraw(t_cal *c);   // CLOCK 모드에서 CAL 모드로 돌아올 때 화면 다시 그리기

int32_t cal_evaluate(char *expr);   


// LCD 1번째 줄(0행)에 현재 입력 중인 수식을 표시
// 버퍼가 16글자를 넘으면 뒤에서부터 16글자만 보여줌(스크롤 효과)
static void lcd_show_expr(t_cal *c)
{
	uint8_t i;
	uint8_t len = strlen(c->buf);
	char *start = (len > 16) ? (c->buf + (len - 16)) : c->buf;
	
	lcd_goto_xy(0, 0);
	for (i = 0; i < 16; i++) lcd_write_data(' ');   // 한 줄 지우기
	
	lcd_goto_xy(0, 0);
	lcd_write_string(start);
}

static uint8_t hex_mode = 0;   // 0=10진수, 1=16진수

// LCD 2번째 줄(1행)에 계산 결과를 표시
static void lcd_show_result(int32_t result)
{
	char resbuf[17];
	uint8_t i;
	
	if (hex_mode)
	snprintf(resbuf, sizeof(resbuf), "0x%lX", (unsigned long)result);
	else
	snprintf(resbuf, sizeof(resbuf), "%ld", result);
	
	lcd_goto_xy(1, 0);
	for (i = 0; i < 16; i++) lcd_write_data(' ');   // 한 줄 지우기
	
	lcd_goto_xy(1, 0);
	lcd_write_string(resbuf);
}

// LCD 2번째 줄(1행)에 에러 메시지를 표시
static void lcd_show_message(char *msg)
{
	uint8_t i;
	
	lcd_goto_xy(1, 0);
	for (i = 0; i < 16; i++) lcd_write_data(' ');   // 한 줄 지우기
	
	lcd_goto_xy(1, 0);
	lcd_write_string(msg);
}

static uint8_t cal_error  = 0;   // 0으로 나누기 등 계산 오류 발생 여부
static int32_t last_result = 0;  // 마지막으로 계산된 결과 (헥사 변환용)
static uint8_t last_valid  = 0;  // last_result가 유효한지 여부

static uint8_t check_parens(char *expr)
{
	int8_t depth = 0;
	int8_t i = 0;
	
	while (expr[i] != '\0')
	{
		if (expr[i] == '(')
		{
			depth++;
		}
		else if (expr[i] == ')')
		{
			depth--;
			if (depth < 0) return 0;   // '(' 없이 ')' 부터 나옴
		}
		i++;
	}
	
	return (depth == 0);
}

static uint8_t precedence(char op)
{
	if (op == '+' || op == '-') return 1;
	if (op == '*' || op == '/') return 2;
	return 0;
}

// 계산 수행
static int32_t apply_op(int32_t a, int32_t b, char op)
{
	switch (op)
	{
		case '+': return a + b;
		case '-': return a - b;
		case '*': return a * b;
		case '/':
		if (b == 0) { printf("ERR: div by zero\n"); cal_error = 1; return 0; }
		return a / b;
	}
	return 0;
}


int32_t cal_evaluate(char *expr)
{
	int32_t num_stack[STACK_SIZE];   // 숫자 스택
	char    op_stack[STACK_SIZE];    // 연산자 스택
	int8_t  num_top = -1;
	int8_t  op_top  = -1;

	int8_t i = 0;
	
	cal_error = 0;   // 이번 계산의 에러 상태 초기화

	while (expr[i] != '\0')
	{
		// 1. 숫자면 파싱해서 num_stack에 push
		if (expr[i] >= '0' && expr[i] <= '9')
		{
			int32_t num = 0;
			while (expr[i] >= '0' && expr[i] <= '9')
			{
				num = num * 10 + (expr[i] - '0');
				i++;
			}
			num_stack[++num_top] = num;
			continue;
		}

		// 2. '(' 이면 op_stack에 push
		if (expr[i] == '(')
		{
			op_stack[++op_top] = expr[i];
			i++;
			continue;
		}

		// 3. ')' 이면 '(' 나올 때까지 계산
		if (expr[i] == ')')
		{
			while (op_top >= 0 && op_stack[op_top] != '(')
			{
				int32_t b = num_stack[num_top--];
				int32_t a = num_stack[num_top--];
				char    op = op_stack[op_top--];
				num_stack[++num_top] = apply_op(a, b, op);
			}
			op_top--;   // '(' 제거
			i++;
			continue;
		}

		// 4. 연산자면 우선순위 비교 후 처리
		if (expr[i]=='+' || expr[i]=='-' || expr[i]=='*' || expr[i]=='/')
		{
			while (op_top >= 0 && op_stack[op_top] != '(' &&
			precedence(op_stack[op_top]) >= precedence(expr[i]))
			{
				int32_t b  = num_stack[num_top--];
				int32_t a  = num_stack[num_top--];
				char    op = op_stack[op_top--];
				num_stack[++num_top] = apply_op(a, b, op);
			}
			op_stack[++op_top] = expr[i];
			i++;
			continue;
		}

		i++;   // 모르는 문자 스킵
	}

	// 5. 남은 연산자 전부 처리
	while (op_top >= 0)
	{
		int32_t b  = num_stack[num_top--];
		int32_t a  = num_stack[num_top--];
		char    op = op_stack[op_top--];
		num_stack[++num_top] = apply_op(a, b, op);
	}

	return num_stack[num_top];
}

void cal_reset(t_cal *c)
{
	memset(c->buf, 0, BUF_SIZE);
	c->idx = 0;
	hex_mode = 0;   // C 누르면 10진수 모드로 리셋
	printf("CLEAR\n");
	lcd_clear();
}

// CLOCK 모드에서 CAL 모드로 돌아올 때, 현재 입력 버퍼 + 마지막 결과를 화면에 다시 그림
void cal_redraw(t_cal *c)
{
	lcd_show_expr(c);
	if (last_valid)
	{
		lcd_show_result(last_result);
	}
}

void cal_input(t_cal *c, uint8_t key)
{
	// Clear
	if (key == 'C')
	{
		cal_reset(c);
		return;
	}

	// '=' 누르면 계산
	if (key == '=')
	{
		if (c->idx == 0)
		{
			printf("error: empty\n");
			return;
		}
		
		if (!check_parens(c->buf))
		{
			printf("ERR: paren mismatch (%s)\n", c->buf);
			lcd_show_message("PAREN ERR");
			return;
		}
		
		printf("%s=", c->buf);
		int32_t result = cal_evaluate(c->buf);
		
		lcd_show_expr(c);        // 1번째 줄: 계산했던 수식
		
		if (cal_error)
		{
			printf("ERR\n");
			lcd_show_message("ERR");
			last_valid = 0;
		}
		else
		{
			printf("%ld\n", result);
			lcd_show_result(result);   // 2번째 줄: 결과
			last_result = result;
			last_valid  = 1;
		}
		
		// 버퍼만 초기화 (결과는 화면에 그대로 남겨둠, 다음 키 입력 시 지워짐)
		memset(c->buf, 0, BUF_SIZE);
		c->idx = 0;
		return;
	}
	
	if (key == 'H')
	{
		hex_mode = !hex_mode;
		
		if (last_valid)
		{
			lcd_show_result(last_result);   // 현재 모드에 맞춰 결과 다시 표시
		}
		else
		{
			lcd_show_message(hex_mode ? "HEX MODE" : "DEC MODE");
		}
		return;
	}
	
	if (key == '\b')
	{
		if (c->idx > 0)
		{
			c->idx--;
			c->buf[c->idx] = '\0';
			printf("%s\n", c->buf);
			lcd_show_expr(c);
		}
		return;
	}
	
	// 버퍼에 추가
	if (c->idx < BUF_SIZE - 1)
	{
		c->buf[c->idx++] = key;
		c->buf[c->idx]   = '\0';
		printf("%s\n", c->buf);   // 현재 입력 상태 출력
		lcd_show_expr(c);         // LCD 1번째 줄에도 표시
	}
	else
	{
		printf("error: buf full\n");
	}
}