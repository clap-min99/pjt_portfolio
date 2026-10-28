`timescale 1ns / 1ps
/*
1. s1 s2 CW     00 10 11 01 00 r_step (순차)
2. s1 s2 CCW    00 01 11 10 00 l_step (순차)
3. key toggle (순차)
4. led display(조합)
*/
module rotary(input clk, reset, s1, s2, key, output [15:0]led);

reg[1:0] r_direction=2'b00; // 시계방향 01 반시계방향 10
reg[1:0] r_prev_state=2'b00; reg[1:0] r_curr_state=2'b00;
reg[1:0] r_step=2'b00; reg[7:0] r_counter = 8'h00;

always @(posedge clk or posedge reset) begin
    if(reset) begin
        r_direction <=2'b00; r_prev_state <=2'b00; r_curr_state <=2'b00; r_counter <= 8'h00; r_step <=2'b00;
    end else begin
        r_prev_state <= r_curr_state;
        r_curr_state <= {s1,s2};
        case({r_prev_state, r_curr_state})
            4'b0010, 4'b1011, 4'b1101, 4'b0100: begin
                if(r_counter < 8'hff && r_step == 2'b11) begin
                    r_counter <= r_counter + 1; 
                end
                r_step <= r_step + 1;
                r_direction <= 2'b01;
            end
            4'b0001, 4'b0111, 4'b1110, 4'b1000: begin
                if(r_counter > 8'h00 && r_step == 2'b11) begin
                    r_counter <= r_counter - 1; 
                end
                r_step <= r_step + 1;
                r_direction <= 2'b10;
            end     
        endcase
    end
end

reg r_led_toggle = 1'b0;
reg r_prev_key=1'b0;
always @(posedge clk, posedge reset) begin
    if(reset) begin
        r_led_toggle <= 0; r_prev_key <= 0;
    end else begin
        r_prev_key <= key;
        if(!r_prev_key && key) r_led_toggle <= ~r_led_toggle;
    end
end

assign led[15:14] = r_direction;
assign led[13] = r_led_toggle;
assign led[12:8] = 5'b00000;
assign led[7:0] = r_counter;
endmodule
