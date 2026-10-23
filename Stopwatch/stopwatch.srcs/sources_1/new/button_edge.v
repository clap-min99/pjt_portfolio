`timescale 1ns / 1ps


module button_edge(
    input clk,
    input reset,
    input [2:0] clean_btn,
    output [2:0] btn_pulse
);

    // 이전 클럭의 버튼 상태 저장
    reg [2:0] prev_btn;

    always @(posedge clk or posedge reset) begin
        if (reset)
            prev_btn <= 3'b000;
        else
            prev_btn <= clean_btn;
    end

    // 상승엣지 검출
    assign btn_pulse = clean_btn & ~prev_btn; // 현재 버튼 1, 이전버튼 0일 때 펄스 만듦

endmodule
