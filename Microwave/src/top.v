`timescale 1ns / 1ps

// Set minutes or seconds with the rotary encoder.
module top(
    input clk,
    input reset,
    input rotary_s1,
    input rotary_s2,
    input rotary_key,
    input btn_start,
    input btn_cancel,
    input btn_door,
    input [1:0] motor_speed,
    output [3:0] an,
    output [7:0] seg,
    output key_led,
    output buzzer,
    output servo_pwm,
    output motor_pwm,
    output motor_in1,
    output motor_in2
);
    wire tick_1ms;
    reg [2:0] rotary_meta, rotary_sync;
    wire s1_clean, s2_clean, key_clean;
    wire [15:0] rotary_led;
    reg [5:0] minutes, seconds;
    reg [1:0] previous_rotary;
    reg signed [3:0] rotary_steps;
    reg running;
    reg display_animation;
    reg finished;
    reg [11:0] finish_ms;
    reg [16:0] buzzer_div;
    reg buzzer_wave;
    reg [7:0] beep_ms;
    reg key_prev;
    reg door_meta, door_sync, door_prev, door_open;
    reg [20:0] servo_count;
    reg [12:0] motor_count;
    reg [1:0] speed_meta, speed_sync;
    reg [12:0] motor_threshold;
    wire door_clean;
    wire door_pulse = door_clean & ~door_prev;
    assign servo_pwm = servo_count < (door_open ? 21'd200_000 : 21'd100_000);
    assign motor_pwm = running && !reset && (motor_count < motor_threshold);
    assign motor_in1 = running && !reset;
    assign motor_in2 = 1'b0;

    // 20 kHz PWM with four selectable duty cycles: 40, 60, 80, 100%.
    always @(*) begin
        case (speed_sync)
            2'b00: motor_threshold = 2000;
            2'b01: motor_threshold = 3000;
            2'b10: motor_threshold = 4000;
            2'b11: motor_threshold = 5000;
        endcase
    end
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            motor_count <= 0;
            speed_meta <= 0;
            speed_sync <= 0;
        end else begin
            speed_meta <= motor_speed;
            speed_sync <= speed_meta;
            if (motor_count == 4999) motor_count <= 0;
            else motor_count <= motor_count + 1'b1;
        end
    end
    reg [26:0] second_count;
    reg [25:0] blink_count;
    reg blink_off;
    reg [6:0] anim_count;
    reg [3:0] anim_step;
    reg [1:0] button_meta, button_sync, button_prev;
    wire start_clean, cancel_clean;
    wire start_pulse = start_clean & ~button_prev[0];
    wire cancel_pulse = cancel_clean & ~button_prev[1];
    wire key_pulse = key_clean & ~key_prev;
    wire [3:0] min_tens = minutes / 6'd10;
    wire [3:0] min_ones = minutes % 6'd10;
    wire [3:0] sec_tens = seconds / 6'd10;
    wire [3:0] sec_ones = seconds % 6'd10;
    wire [15:0] display_value = {min_tens, min_ones, sec_tens, sec_ones};
    wire finish_blank = finished && ((finish_ms >= 500 && finish_ms < 1000) ||
                                      (finish_ms >= 1500 && finish_ms < 2000) ||
                                      (finish_ms >= 2500));
    wire finish_beep = finished && !finish_blank;
    assign buzzer = (beep_ms != 0 || finish_beep) && buzzer_wave;
    wire [15:0] visible_value = finished ? (finish_blank ? 16'hffff : 16'h0000) :
        (!running && blink_off)
        ? (key_led ? {min_tens, min_ones, 8'hff}
                   : {8'hff, sec_tens, sec_ones})
        : display_value;

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            blink_count <= 0;
            blink_off <= 0;
        end else if (running) begin
            blink_count <= 0;
            blink_off <= 0;
        end else if (blink_count == 49_999_999) begin
            blink_count <= 0;
            blink_off <= ~blink_off;
        end else blink_count <= blink_count + 1'b1;
    end

    // 2 kHz tone at 100 MHz and a 100 ms beep for button presses.
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            buzzer_div <= 0;
            buzzer_wave <= 0;
            beep_ms <= 0;
            key_prev <= 0;
        end else begin
            key_prev <= key_clean;
            if (buzzer_div == 24_999) begin
                buzzer_div <= 0;
                buzzer_wave <= ~buzzer_wave;
            end else buzzer_div <= buzzer_div + 1'b1;
            if (start_pulse || cancel_pulse || key_pulse || door_pulse)
                beep_ms <= 100;
            else if (tick_1ms && beep_ms != 0)
                beep_ms <= beep_ms - 1'b1;
        end
    end

    // 20 ms period; 1 ms closed and 2 ms open at 100 MHz.
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            door_meta <= 0;
            door_sync <= 0;
            door_prev <= 0;
            door_open <= 0;
            servo_count <= 0;
        end else begin
            door_meta <= btn_door;
            door_sync <= door_meta;
            door_prev <= door_clean;
            if (door_pulse && !running && !finished) door_open <= ~door_open;
            if (servo_count == 1_999_999) servo_count <= 0;
            else servo_count <= servo_count + 1'b1;
        end
    end
    debouncer u_door (
        .clk(clk), .reset(reset), .noisy_btn(door_sync), .clean_btn(door_clean)
    );

    // Advance the twelve-segment animation every 100 ms while cooking.
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            anim_count <= 0;
            anim_step <= 0;
        end else if (!running) begin
            anim_count <= 0;
            anim_step <= 0;
        end else if (tick_1ms) begin
            if (anim_count == 99) begin
                anim_count <= 0;
                if (anim_step == 11) anim_step <= 0;
                else anim_step <= anim_step + 1'b1;
            end else anim_count <= anim_count + 1'b1;
        end
    end

    always @(posedge clk or posedge reset) begin
        if (reset) begin
            rotary_meta <= 0;
            rotary_sync <= 0;
        end else begin
            rotary_meta <= {rotary_key, rotary_s2, rotary_s1};
            rotary_sync <= rotary_meta;
        end
    end

    debouncer #(.DEBOUNCE_LIMIT(200_000)) u_s1 (
        .clk(clk), .reset(reset), .noisy_btn(rotary_sync[0]), .clean_btn(s1_clean)
    );
    debouncer #(.DEBOUNCE_LIMIT(200_000)) u_s2 (
        .clk(clk), .reset(reset), .noisy_btn(rotary_sync[1]), .clean_btn(s2_clean)
    );
    debouncer u_key (
        .clk(clk), .reset(reset), .noisy_btn(rotary_sync[2]), .clean_btn(key_clean)
    );
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            button_meta <= 0;
            button_sync <= 0;
            button_prev <= 0;
        end else begin
            button_meta <= {btn_cancel, btn_start};
            button_sync <= button_meta;
            button_prev <= {cancel_clean, start_clean};
        end
    end
    debouncer u_start (
        .clk(clk), .reset(reset), .noisy_btn(button_sync[0]), .clean_btn(start_clean)
    );
    debouncer u_cancel (
        .clk(clk), .reset(reset), .noisy_btn(button_sync[1]), .clean_btn(cancel_clean)
    );
    rotary u_rotary (
        // Swap the quadrature inputs so clockwise increases the count.
        .clk(clk), .reset(reset), .s1(s2_clean), .s2(s1_clean),
        .key(key_clean), .led(rotary_led)
    );
    assign key_led = rotary_led[13];

    // Count four valid quadrature transitions per detent, reversing direction
    // from the original rotary module to match the physical knob.
    always @(posedge clk or posedge reset) begin
        if (reset) begin
            minutes <= 0;
            seconds <= 0;
            previous_rotary <= 0;
            rotary_steps <= 0;
            running <= 0;
            second_count <= 0;
            display_animation <= 0;
            finished <= 0;
            finish_ms <= 0;
        end else begin
            previous_rotary <= {s2_clean, s1_clean};
            if (cancel_pulse) begin
                minutes <= 0;
                seconds <= 0;
                running <= 0;
                second_count <= 0;
                rotary_steps <= 0;
                display_animation <= 0;
                finished <= 0;
                finish_ms <= 0;
            end else if (finished) begin
                rotary_steps <= 0;
                if (tick_1ms) begin
                    if (finish_ms == 2999) begin
                        finished <= 0;
                        finish_ms <= 0;
                    end else finish_ms <= finish_ms + 1'b1;
                end
            end else if (running) begin
                rotary_steps <= 0;
                if (second_count == 99_999_999) begin
                    second_count <= 0;
                    display_animation <= ~display_animation;
                    if (seconds != 0) begin
                        seconds <= seconds - 1'b1;
                        if (minutes == 0 && seconds == 1) begin
                            running <= 0;
                            finished <= 1;
                            finish_ms <= 0;
                        end
                    end else if (minutes != 0) begin
                        minutes <= minutes - 1'b1;
                        seconds <= 59;
                    end
                end else second_count <= second_count + 1'b1;
            end else if (start_pulse && !door_open && (minutes != 0 || seconds != 0)) begin
                running <= 1;
                second_count <= 0;
                display_animation <= 0;
                rotary_steps <= 0;
            end else begin
            case ({previous_rotary, s2_clean, s1_clean})
                4'b0010, 4'b1011, 4'b1101, 4'b0100: begin
                    if (rotary_steps == 3) begin
                        rotary_steps <= 0;
                        if (key_led) begin
                            if (seconds < 59) seconds <= seconds + 1'b1;
                        end else if (minutes < 59) minutes <= minutes + 1'b1;
                    end else rotary_steps <= rotary_steps + 1'b1;
                end
                4'b0001, 4'b0111, 4'b1110, 4'b1000: begin
                    if (rotary_steps == -3) begin
                        rotary_steps <= 0;
                        if (key_led) begin
                            if (seconds > 0) seconds <= seconds - 1'b1;
                        end else if (minutes > 0) minutes <= minutes - 1'b1;
                    end else rotary_steps <= rotary_steps - 1'b1;
                end
                default: ;
            endcase
            end
        end
    end

    tick_generator #(.TICK_Hz(1000)) u_tick_1ms (
        .clk(clk), .reset(reset), .tick(tick_1ms)
    );

    fnd_controller u_fnd (
        .clk(clk), .reset(reset), .tick_1ms(tick_1ms),
        .bcd_data(visible_value), .dot_en(finish_blank ? 4'b0000 : 4'b0100),
        .anim_active(running && display_animation), .anim_step(anim_step),
        .an(an), .seg(seg)
    );
endmodule
