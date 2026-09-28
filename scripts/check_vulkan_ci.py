#!/usr/bin/env python3
"""Stage 0 headless-Vulkan check (ROADMAP Stage 0).

Selects the lavapipe ICD explicitly via VK_DRIVER_FILES, then requires that vulkaninfo reports
the llvmpipe device and lists VK_LAYER_KHRONOS_validation. Diagnostics: AX-VK-001..003.
"""
from __future__ import annotations

import glob
import os
import subprocess
import sys


def main() -> int:
    icds = sorted(glob.glob("/usr/share/vulkan/icd.d/lvp_icd*.json"))
    if not icds:
        print("AX-VK-001 lavapipe ICD json not found under /usr/share/vulkan/icd.d/")
        return 1
    env = dict(os.environ, VK_DRIVER_FILES=icds[0], VK_ICD_FILENAMES=icds[0])
    summary = subprocess.run(["vulkaninfo", "--summary"], capture_output=True, text=True, env=env)
    full = subprocess.run(["vulkaninfo"], capture_output=True, text=True, env=env)
    text = summary.stdout + summary.stderr
    print(f"ICD: {icds[0]}")
    print("\n".join(line for line in text.splitlines() if "deviceName" in line or "driverName" in line))
    if summary.returncode != 0 or "llvmpipe" not in text:
        print(f"AX-VK-002 vulkaninfo did not report llvmpipe (exit {summary.returncode})")
        print("\n".join(text.splitlines()[:40]))
        return 1
    if "VK_LAYER_KHRONOS_validation" not in text + full.stdout:
        print("AX-VK-003 VK_LAYER_KHRONOS_validation is not listed")
        return 1
    print("check_vulkan_ci: lavapipe selected, llvmpipe reported, validation layer present")
    return 0


if __name__ == "__main__":
    sys.exit(main())
