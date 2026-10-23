
`timescale 1ns / 1ps

module tb_tick_generator;

    reg clk = 0;
    reg reset = 0;
    wire tick;

    // 100MHz Clock (10ns period)
    always #5 clk = ~clk;

    // DUT
    tick_generator #(
        .INPUT_FREQ(100),
        .TICK_Hz(10)
    ) uut (
        .clk(clk),
        .reset(reset),
        .tick(tick)
    );

    initial begin
        // Reset
        reset = 1;
        #20;
        reset = 0;

        // Run simulation
        #350;

        $finish;
    end

endmodule
