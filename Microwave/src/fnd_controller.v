
`timescale 1ns / 1ps

module fnd_controller (
    input clk,
    input reset,
    input tick_1ms,

    input [15:0] bcd_data,
    input [3:0] dot_en,

    input anim_active,
    input [3:0] anim_step,

    output reg [3:0] an,
    output reg [7:0] seg
);

    reg [1:0] digit_sel;
    reg [3:0] digit_data;
    reg [6:0] seg_pattern;

    // 1. FND Digit Sca
    always @(posedge clk or posedge reset) begin
        if (reset)
            digit_sel <= 2'd0;
        else if (tick_1ms)
            digit_sel <= digit_sel + 1'b1;
    end

    // 2. Select BCD Digit
    always @(*) begin
        case (digit_sel)
            2'd0: digit_data = bcd_data[3:0];
            2'd1: digit_data = bcd_data[7:4];
            2'd2: digit_data = bcd_data[11:8];
            2'd3: digit_data = bcd_data[15:12];
            default: digit_data = 4'd0;
        endcase
    end

    // 3. BCD -> 7 Segment (Active Low)
    always @(*) begin
        case (digit_data)
            4'd0: seg_pattern = 7'b1000000;
            4'd1: seg_pattern = 7'b1111001;
            4'd2: seg_pattern = 7'b0100100;
            4'd3: seg_pattern = 7'b0110000;
            4'd4: seg_pattern = 7'b0011001;
            4'd5: seg_pattern = 7'b0010010;
            4'd6: seg_pattern = 7'b0000010;
            4'd7: seg_pattern = 7'b1111000;
            4'd8: seg_pattern = 7'b0000000;
            4'd9: seg_pattern = 7'b0010000;
            default: seg_pattern = 7'b1111111;
        endcase
    end

    // 4. Normal Display / Circle Animation
    always @(*) begin
        an  = 4'b1111;
        seg = 8'b11111111;

        // Current digit selection
        case (digit_sel)
            2'd0: an = 4'b1110;
            2'd1: an = 4'b1101;
            2'd2: an = 4'b1011;
            2'd3: an = 4'b0111;
            default: an = 4'b1111;
        endcase

        if (anim_active) begin

            // Circle Animation: 12 steps
            // Top: AN3 -> AN0
            // Right: AN0 b, c
            // Bottom: AN0 -> AN3
            // Left: AN3 e, f

            case (anim_step)
                4'd0:  if (digit_sel == 3) seg[0] = 0;
                4'd1:  if (digit_sel == 2) seg[0] = 0;
                4'd2:  if (digit_sel == 1) seg[0] = 0;
                4'd3:  if (digit_sel == 0) seg[0] = 0;

                4'd4:  if (digit_sel == 0) seg[1] = 0;
                4'd5:  if (digit_sel == 0) seg[2] = 0;

                4'd6:  if (digit_sel == 0) seg[3] = 0;
                4'd7:  if (digit_sel == 1) seg[3] = 0;
                4'd8:  if (digit_sel == 2) seg[3] = 0;
                4'd9:  if (digit_sel == 3) seg[3] = 0;

                4'd10: if (digit_sel == 3) seg[4] = 0;
                4'd11: if (digit_sel == 3) seg[5] = 0;

                default: seg = 8'b11111111;
            endcase

            // Dot OFF
            seg[7] = 1'b1;
        end
        else begin
            // Normal Clock / Stopwatch display
            seg[6:0] = seg_pattern;
            seg[7]   = ~dot_en[digit_sel];
        end
    end

endmodule
