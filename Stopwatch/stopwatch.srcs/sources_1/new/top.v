
`timescale 1ns / 1ps

module top(
    input clk,
    input reset,
    input [2:0] btn,

    output [3:0] an,
    output [7:0] seg
);

    // =========================================
    // 1. Button Debouncer
    // =========================================

    wire [2:0] w_clean_btn;
    wire [2:0] w_btn_pulse;

    btn_debouncer u_btn_debouncer (
        .clk(clk),
        .reset(reset),
        .noisy_btn(btn),
        .clean_btn(w_clean_btn)
    );

    // =========================================
    // 2. Button Edge Detector
    // =========================================

    button_edge u_button_edge (
        .clk(clk),
        .reset(reset),
        .clean_btn(w_clean_btn),
        .btn_pulse(w_btn_pulse)
    );

    // =========================================
    // 3. Mode Controller (FSM)
    // =========================================

    wire [1:0] w_mode;
    wire w_sw_run;
    wire w_sw_clear;

    mode_controller u_mode_controller (
        .clk(clk),
        .reset(reset),

        .btn_left(w_btn_pulse[0]),
        .btn_center(w_btn_pulse[1]),
        .btn_right(w_btn_pulse[2]),

        .mode(w_mode),
        .sw_run(w_sw_run),
        .sw_clear(w_sw_clear)
    );

    // =========================================
    // 4. Tick Generator
    // =========================================

    wire w_tick_10ms;
    wire w_tick_1ms;

    // 10ms Tick: Stopwatch / Clock
    tick_generator #(
        .TICK_Hz(100)
    ) u_tick_10ms (
        .clk(clk),
        .reset(reset),
        .tick(w_tick_10ms)
    );

    // 1ms Tick: FND Scanning
    tick_generator #(
        .TICK_Hz(1000)
    ) u_tick_1ms (
        .clk(clk),
        .reset(reset),
        .tick(w_tick_1ms)
    );

    // =========================================
    // 5. Time Counter
    // =========================================

    wire [5:0] w_clock_min;
    wire [5:0] w_clock_sec;

    wire [5:0] w_stopwatch_sec;
    wire [6:0] w_stopwatch_cs;

    time_counter u_time_counter (
        .clk(clk),
        .reset(reset),
        .tick_10ms(w_tick_10ms),

        .sw_run(w_sw_run),
        .sw_clear(w_sw_clear),

        .clock_min(w_clock_min),
        .clock_sec(w_clock_sec),

        .stopwatch_sec(w_stopwatch_sec),
        .stopwatch_cs(w_stopwatch_cs)
    );

    // =========================================
    // 6. Binary to BCD
    // =========================================

    wire [3:0] cm10, cm1, cs10, cs1;
    wire [3:0] ss10, ss1, sc10, sc1;

    // Clock: MM:SS
    assign cm10 = w_clock_min / 10;
    assign cm1  = w_clock_min % 10;

    assign cs10 = w_clock_sec / 10;
    assign cs1  = w_clock_sec % 10;

    // Stopwatch: SS.CC
    assign ss10 = w_stopwatch_sec / 10;
    assign ss1  = w_stopwatch_sec % 10;

    assign sc10 = w_stopwatch_cs / 10;
    assign sc1  = w_stopwatch_cs % 10;

    wire [15:0] w_clock_bcd;
    wire [15:0] w_stopwatch_bcd;

    assign w_clock_bcd = {
        cm10, cm1, cs10, cs1
    };

    assign w_stopwatch_bcd = {
        ss10, ss1, sc10, sc1
    };

    // =========================================
    // 7. Display Mode Selection
    // =========================================

    wire [15:0] w_display_bcd;

    // S_CLOCK = 00
    // S_STOPWATCH = 01
    // S_PAUSE = 10

    assign w_display_bcd = (w_mode == 2'b00)
                         ? w_clock_bcd
                         : w_stopwatch_bcd;

    // =========================================
    // 8. Dot Blink Control
    // =========================================

    reg r_dot_blink;
    reg [6:0] r_dot_tick_count;

    // 10ms x 100 = 1sec
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            r_dot_blink <= 1'b0;
            r_dot_tick_count <= 0;
        end
        else if (w_tick_10ms) begin
            if (r_dot_tick_count == 99) begin
                r_dot_tick_count <= 0;
                r_dot_blink <= ~r_dot_blink;
            end
            else begin
                r_dot_tick_count <= r_dot_tick_count + 1'b1;
            end
        end
    end

    // =========================================
    // 9. Dot Position Selection
    // =========================================

    wire [3:0] w_dot_en;

    // CLOCK: MM.SS (Blink)
    // STOPWATCH / PAUSE: SS.CC (Always ON)

assign w_dot_en = (w_mode == 2'b00)
                ? {1'b0, r_dot_blink, 2'b00}
                : 4'b0100;

    
    // =========================================
    // 10. Animation Controller
    // =========================================

    wire w_anim_active;
    wire [3:0] w_anim_step;

    animation_controller u_animation_controller (
        .clk(clk),
        .reset(reset),
        .tick_10ms(w_tick_10ms),

        .mode(w_mode),
        .btn_pulse(w_btn_pulse),

        .anim_active(w_anim_active),
        .anim_step(w_anim_step)
    );

    // =========================================
    // 11. FND Controller
    // =========================================

    fnd_controller u_fnd_controller (
        .clk(clk),
        .reset(reset),
        .tick_1ms(w_tick_1ms),

        .bcd_data(w_display_bcd),
        .dot_en(w_dot_en),

        .anim_active(w_anim_active),
        .anim_step(w_anim_step),

        .an(an),
        .seg(seg)
    );


endmodule
