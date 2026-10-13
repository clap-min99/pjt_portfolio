"""Test PWM selection and reload without accessing hardware."""
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch, call
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from load_motor import select_pwm_id, reload_motor

records = [("pwmchip0", "3280000.pwm", 1),
           ("pwmchip3", "32c0000.pwm", 1),
           ("pwmchip4", "32e0000.pwm", 1)]
assert select_pwm_id(records) == 3
assert select_pwm_id([("pwmchip8", "32c0000.pwm", 1)]) == 8
for invalid in ([], records[:1], records + [records[1]],
                [("pwmchip3", "32c0000.pwm", 2)]):
    try:
        select_pwm_id(invalid)
    except ValueError:
        pass
    else:
        raise AssertionError("Invalid PWM layout must fail before unloading")
with patch("load_motor.subprocess.run") as run:
    reload_motor(Path("fan_pwm_drv.ko"), 3, True)
    assert run.call_args_list == [call(["rmmod", "fan_pwm_drv"], check=True),
                                 call(["insmod", "fan_pwm_drv.ko", "pwm_id=3"], check=True)]
with patch("load_motor.subprocess.run", side_effect=subprocess.CalledProcessError(1, "rmmod")) as run:
    try:
        reload_motor(Path("fan_pwm_drv.ko"), 3, True)
    except subprocess.CalledProcessError:
        pass
    else:
        raise AssertionError("Reload must stop if module removal fails")
    assert run.call_count == 1
print("PASS: ENA pin 33 PWM selection, variable IDs, reload order and failure handling")
