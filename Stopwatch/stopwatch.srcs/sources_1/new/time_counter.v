`timescale 1ns / 1ps

module time_counter (
    input clk,
    input reset,
    input tick_10ms,

    input sw_run,
    input sw_clear,

    output reg [5:0] clock_min,
    output reg [5:0] clock_sec,

    output reg [5:0] stopwatch_sec,
    output reg [6:0] stopwatch_cs
);

    // 10ms Tick을 100번 세면 1초
    reg [6:0] clock_tick_count;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            clock_tick_count <= 0;
            clock_min <= 0;
            clock_sec <= 0;
        end
        else if (tick_10ms) begin
            if (clock_tick_count == 99) begin
                clock_tick_count <= 0;

                if (clock_sec == 59) begin
                    clock_sec <= 0;

                    if (clock_min == 59)
                        clock_min <= 0;
                    else
                        clock_min <= clock_min + 1;
                end
                else begin
                    clock_sec <= clock_sec + 1;
                end
            end
            else begin
                clock_tick_count <= clock_tick_count + 1;
            end
        end
    end

    // Stopwatch: 10ms 단위 카운트
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            stopwatch_sec <= 0;
            stopwatch_cs <= 0;
        end
        else if (sw_clear) begin
            stopwatch_sec <= 0;
            stopwatch_cs <= 0;
        end
        else if (sw_run && tick_10ms) begin
            if (stopwatch_cs == 99) begin
                stopwatch_cs <= 0;

                if (stopwatch_sec == 59)
                    stopwatch_sec <= 0;
                else
                    stopwatch_sec <= stopwatch_sec + 1;
            end
            else begin
                stopwatch_cs <= stopwatch_cs + 1;
            end
        end
    end

endmodule
