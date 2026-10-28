`timescale 1ns / 1ps

module tb_microwave;
    reg clk = 0, reset = 1;
    reg rotary_s1 = 0, rotary_s2 = 0, rotary_key = 0;
    reg btn_start = 0, btn_cancel = 0, btn_door = 0;
    reg [1:0] motor_speed = 0;
    wire [3:0] an;
    wire [7:0] seg;
    wire key_led, buzzer, servo_pwm, motor_pwm, motor_in1, motor_in2;
    integer errors = 0;

    top dut (
        .clk(clk), .reset(reset), .rotary_s1(rotary_s1), .rotary_s2(rotary_s2),
        .rotary_key(rotary_key), .btn_start(btn_start), .btn_cancel(btn_cancel),
        .btn_door(btn_door), .motor_speed(motor_speed), .an(an), .seg(seg),
        .key_led(key_led), .buzzer(buzzer), .servo_pwm(servo_pwm),
        .motor_pwm(motor_pwm), .motor_in1(motor_in1), .motor_in2(motor_in2)
    );
    always #5 clk = ~clk;

    task check;
        input condition;
        input [255:0] message;
        begin
            if (condition !== 1'b1) begin
                $display("FAIL: %0s at %0t", message, $time);
                errors = errors + 1;
            end else $display("PASS: %0s", message);
        end
    endtask

    // Drive debounced signals directly to avoid waiting for mechanical debounce.
    task press_start;
        begin
            force dut.start_clean = 1; @(posedge clk); #1;
            force dut.start_clean = 0; @(posedge clk); #1;
        end
    endtask
    task press_cancel;
        begin
            force dut.cancel_clean = 1; @(posedge clk); #1;
            force dut.cancel_clean = 0; @(posedge clk); #1;
        end
    endtask
    task press_door;
        begin
            force dut.door_clean = 1; @(posedge clk); #1;
            force dut.door_clean = 0; @(posedge clk); #1;
        end
    endtask
    task advance_second;
        begin
            @(negedge clk); force dut.second_count = 99_999_999;
            @(posedge clk); #1; release dut.second_count;
            @(negedge clk); dut.second_count = 0;
        end
    endtask
    // A full clockwise quadrature cycle using the same swapped input order as top.
    task rotary_increment;
        begin
            force dut.s2_clean = 1; force dut.s1_clean = 0; repeat (2) @(posedge clk); #1;
            force dut.s2_clean = 1; force dut.s1_clean = 1; repeat (2) @(posedge clk); #1;
            force dut.s2_clean = 0; force dut.s1_clean = 1; repeat (2) @(posedge clk); #1;
            force dut.s2_clean = 0; force dut.s1_clean = 0; repeat (2) @(posedge clk); #1;
        end
    endtask

    initial begin
        force dut.start_clean = 0; force dut.cancel_clean = 0; force dut.door_clean = 0;
        force dut.s1_clean = 0; force dut.s2_clean = 0; force dut.key_clean = 0;
        repeat (4) @(posedge clk); #1; reset = 0;
        repeat (3) @(posedge clk); #1;
        check(dut.minutes == 0 && dut.seconds == 0 && !dut.running, "reset clears timer");
        press_start();
        check(!dut.running, "zero time cannot start");

        rotary_increment();
        check(dut.minutes == 1, "rotary sets minutes");
        force dut.key_clean = 1; @(posedge clk); #1;
        force dut.key_clean = 0; repeat (2) @(posedge clk); #1;
        check(key_led == 1, "rotary key selects seconds");
        rotary_increment();
        check(dut.seconds == 1, "rotary sets seconds");

        press_door();
        check(dut.door_open, "door opens");
        press_start();
        check(!dut.running, "open door prevents start");
        press_door();
        check(!dut.door_open, "door closes");
        press_start();
        check(dut.running && motor_in1 && !motor_in2, "start enables motor direction");
        press_door();
        check(!dut.door_open, "door button ignored while running");
        advance_second();
        check(dut.minutes == 1 && dut.seconds == 0, "countdown decrements seconds");
        advance_second();
        check(dut.minutes == 0 && dut.seconds == 59, "countdown borrows minute");
        press_cancel();
        check(!dut.running && !motor_in1 && !motor_pwm && dut.minutes == 0 && dut.seconds == 0,
              "cancel stops motor and clears time");

        rotary_increment();
        press_start();
        check(dut.running, "restart with one second");
        advance_second();
        check(!dut.running && dut.finished && !motor_pwm && dut.seconds == 0,
              "timeout stops motor and starts finish");
        check(!dut.finish_blank && dut.finish_beep, "first finish alert active");
        @(negedge clk); force dut.finish_ms = 500; #1;
        check(dut.finish_blank && !dut.finish_beep, "finish alert blank interval");
        release dut.finish_ms;
        press_cancel();
        check(!dut.finished && !dut.running, "cancel dismisses finish alert");
        if (errors == 0) $display("ALL TESTS PASSED");
        else $display("TEST FAILED: %0d error(s)", errors);
        $finish;
    end
endmodule
