"""Load ENA=BOARD 33, IN1=BOARD 29 motor PWM on Linux 5.15."""
import argparse
import os
from pathlib import Path
import platform
import re
import subprocess
import sys

CONTROLLER = "32c0000.pwm"


def select_pwm_id(records):
    """Records: (pwmchip name, platform device name, channel count)."""
    matches = []
    for name, device, count in records:
        if device == CONTROLLER:
            match = re.fullmatch(r"pwmchip([0-9]+)", name)
            if not match or count != 1:
                raise ValueError("Unexpected pin-33 PWM chip layout")
            # Linux 5.15 names pwmchip<global base>; channel index is 0.
            matches.append(int(match.group(1)))
    if len(matches) != 1:
        raise ValueError("Expected one 32c0000.pwm controller for ENA pin 33")
    return matches[0]


def reload_motor(module_path, pwm_id, loaded):
    if loaded:
        subprocess.run(["rmmod", "fan_pwm_drv"], check=True)
    subprocess.run(["insmod", str(module_path), f"pwm_id={pwm_id}"], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--print-id", action="store_true",
                        help="Only print the PWM ID; do not reload")
    args = parser.parse_args()
    if not platform.release().startswith("5.15."):
        raise ValueError("This loader targets the board's Linux 5.15 PWM numbering")
    records = []
    for chip in Path("/sys/class/pwm").glob("pwmchip*"):
        records.append((chip.name, (chip / "device").resolve().name,
                        int((chip / "npwm").read_text().strip())))
    pwm_id = select_pwm_id(records)
    if args.print_id:
        print(pwm_id)
        return
    module_path = Path(__file__).resolve().parents[1] / "kernel/fan_pwm/fan_pwm_drv.ko"
    if not module_path.is_file():
        raise ValueError("Build first: make -C kernel/fan_pwm")
    if os.geteuid() != 0:
        raise ValueError("Run with sudo; stop smart-fan before reloading")
    print(f"ENA=BOARD 33 ({CONTROLLER}), IN1=BOARD 29, pwm_id={pwm_id}", flush=True)
    reload_motor(module_path, pwm_id, Path("/sys/module/fan_pwm_drv").is_dir())


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Motor driver load failed: {error}", file=sys.stderr)
        sys.exit(1)
