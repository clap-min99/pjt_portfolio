`timescale 1ns / 1ps

module mode_controller(
    input clk,
    input reset,

    input btn_left,
    input btn_center,
    input btn_right,

    output [1:0] mode,
    output sw_run,
    output sw_clear
);

  // FSM State Definition
    localparam S_CLOCK     = 2'b00;
    localparam S_STOPWATCH = 2'b01;
    localparam S_PAUSE     = 2'b10;

    // State Registers
    reg [1:0] state;
    reg [1:0] next_state;

    // State Register
    always @(posedge clk or posedge reset) begin
        if (reset)
            state <= S_CLOCK;
        else
            state <= next_state;
    end
    
    // Next State Logic
    always @(*) begin
        next_state = state;  // 기본값: 현재 상태 유지

        case (state)
            S_CLOCK: begin
                if (btn_left)
                    next_state = S_STOPWATCH;
            end

            S_STOPWATCH: begin
                if (btn_left)
                    next_state = S_CLOCK;
                else if (btn_right)
                    next_state = S_PAUSE;
            end

            S_PAUSE: begin
                if (btn_right)
                    next_state = S_STOPWATCH;
            end

            default: begin
                next_state = S_CLOCK;
            end
        endcase
    end

    
    // Output Logic
    assign mode = state;

    assign sw_run = (state == S_STOPWATCH);

    assign sw_clear = (state == S_STOPWATCH) && btn_center;

endmodule
