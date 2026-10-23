`timescale 1ns / 1ps

module tb_stopwatch();
    reg clk = 0;
    reg reset = 0;

    reg btn_left = 0;
    reg btn_center = 0;
    reg btn_right = 0;

    wire [1:0] mode;
    wire sw_run;
    wire sw_clear;

    // 100MHz Clock (10ns)
    always #5 clk = ~clk;

    // DUT: Device Under Test
    mode_controller u_mode_controller (
        .clk(clk),
        .reset(reset),
        .btn_left(btn_left),
        .btn_center(btn_center),
        .btn_right(btn_right),
        .mode(mode),
        .sw_run(sw_run),
        .sw_clear(sw_clear)
    );

    // 1클럭 버튼 펄스 생성
    task press_left;
    begin
        @(negedge clk);
        btn_left = 1;
        @(negedge clk);
        btn_left = 0;
    end
    endtask

    task press_center;
    begin
        @(negedge clk);
        btn_center = 1;
        @(negedge clk);
        btn_center = 0;
    end
    endtask

    task press_right;
    begin
        @(negedge clk);
        btn_right = 1;
        @(negedge clk);
        btn_right = 0;
    end
    endtask

    initial begin
        // 1. Reset
        reset = 1;
        #20;
        reset = 0;
        #20;

        // 2. CLOCK -> STOPWATCH
        press_left();
        #20;

        // 3. STOPWATCH -> Clear
        press_center();
        #20;

        // 4. STOPWATCH -> PAUSE
        press_right();
        #20;

        // 5. PAUSE -> STOPWATCH
        press_right();
        #20;

        // 6. STOPWATCH -> CLOCK
        press_left();
        #20;

        $display("FSM simulation finished");
        $finish;
    end
endmodule
