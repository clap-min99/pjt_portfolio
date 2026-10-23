
`timescale 1ns / 1ps

module tb_button_edge;

    reg clk = 0;
    reg reset = 0;
    reg [2:0] clean_btn = 3'b000;

    wire [2:0] btn_pulse;

    always #5 clk = ~clk;

    button_edge uut (
        .clk(clk),
        .reset(reset),
        .clean_btn(clean_btn),
        .btn_pulse(btn_pulse)
    );

    initial begin
        reset = 1;
        #20;
        reset = 0;

        // Left 버튼을 길게 누름
        @(negedge clk);
        clean_btn[0] = 1;
        repeat(5) @(negedge clk);
        clean_btn[0] = 0;

        // Center 버튼
        repeat(2) @(negedge clk);
        clean_btn[1] = 1;
        repeat(5) @(negedge clk);
        clean_btn[1] = 0;

        // Right 버튼
        repeat(2) @(negedge clk);
        clean_btn[2] = 1;
        repeat(5) @(negedge clk);
        clean_btn[2] = 0;

        #20;
        $finish;
    end

endmodule
