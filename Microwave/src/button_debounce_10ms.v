module btn_debouncer(
    input clk,
    input reset,
    input [2:0] noisy_btn,
    output [2:0] clean_btn
);

    debouncer u_btnL_debouncer (
        .clk(clk), 
        .reset(reset),
        .noisy_btn(noisy_btn[0]),
        .clean_btn(clean_btn[0])
    );

    debouncer u_btnC_debouncer (
        .clk(clk), 
        .reset(reset),
        .noisy_btn(noisy_btn[1]),
        .clean_btn(clean_btn[1])
    );

    debouncer u_btnR_debouncer (
        .clk(clk), 
        .reset(reset),
        .noisy_btn(noisy_btn[2]),
        .clean_btn(clean_btn[2])
    );    
endmodule

// 아래에 있는 코드는 버튼 디바운서 이다. 
// 이는 noisy_btn 버튼의 입력에서 발생할 수 있는 
// 노이즈(채터링)을 제거해, 안정적인 버튼 입력(clean_btn)으로 만들어준다.
// 파라미터는 보통 클럭 카운트 제한값으로 사용되어,
// 얼마나 오랫동안 입력이 안정적인지를 판단하기 위한 시간 지연 설정이다.
// 예: 100MHz 클럭이라면, 999,999는 약 10ms의 디바운싱 지연을 의미할 수 있다.
module debouncer #(parameter DEBOUNCE_LIMIT = 20'd999_999) (
    input      clk,
    input      reset,
    input      noisy_btn,  // raw noisy button input
    output reg clean_btn
);
    reg [19:0] r_count;
    reg r_btn_state=0;

    always @(posedge clk or posedge reset) begin
        if (reset) begin   // active-high reset
            r_count <= 0;
            r_btn_state <= 0;
            clean_btn <= 0;
        end else if (noisy_btn == r_btn_state) begin  // 버튼 상태가 이전과 동일할 경우 (안정됨)
            r_count <= 0;
        end else begin
            if (r_count < DEBOUNCE_LIMIT)  // 버튼 상태가 바뀌었지만 아직 안정되지 않은 경우
                r_count <= r_count + 1;
            else begin  // 상태가 충분히 오랫동안 유지됨(10ms)
                r_btn_state <= noisy_btn;
                clean_btn <= noisy_btn;
                r_count <= 0;  // clear하면 다음 변경을 다시 감지할 수 있음
            end
        end
    end
endmodule