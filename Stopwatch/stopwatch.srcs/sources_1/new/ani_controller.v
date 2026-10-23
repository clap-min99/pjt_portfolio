
`timescale 1ns / 1ps

module animation_controller(
    input clk,
    input reset,
    input tick_10ms,
    input [1:0] mode,
    input [2:0] btn_pulse,

    output reg anim_active,
    output reg [3:0] anim_step
);

    localparam S_PAUSE = 2'b10;

    // 10ms x 500 = 5 seconds
    reg [8:0] idle_count;

    // 10ms x 10 = 100ms
    reg [3:0] anim_count;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            idle_count  <= 0;
            anim_count  <= 0;
            anim_active <= 0;
            anim_step   <= 0;
        end
        else if (mode != S_PAUSE || (|btn_pulse)) begin
            idle_count  <= 0;
            anim_count  <= 0;
            anim_active <= 0;
            anim_step   <= 0;
        end
        else if (tick_10ms) begin
            if (!anim_active) begin
                if (idle_count == 499) begin
                    idle_count  <= 0;
                    anim_active <= 1;
                end
                else begin
                    idle_count <= idle_count + 1'b1;
                end
            end
            else begin
                if (anim_count == 9) begin
                    anim_count <= 0;

                    if (anim_step == 11)
                        anim_step <= 0;
                    else
                        anim_step <= anim_step + 1'b1;
                end
                else begin
                    anim_count <= anim_count + 1'b1;
                end
            end
        end
    end

endmodule
