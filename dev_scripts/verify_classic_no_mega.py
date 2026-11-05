#!/usr/bin/env python3
"""Check the actual preprocessed species/form data for Classic and test builds."""
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
compiler = shutil.which("arm-none-eabi-gcc") or str(
    Path(os.environ.get("DEVKITARM", "/opt/devkitpro/devkitARM")) / "bin/arm-none-eabi-gcc"
)
base = [compiler, "-E", "-I", "include", "-I", "build/assets", "-I", "tools/agbcc/include",
        "-DMODERN=1", "-DEMERALD", "-std=gnu17"]

for label, flags, enabled in [
    ("Classic normal", ["-DTESTING=0"], False),
    ("Classic release", ["-DTESTING=0", "-DRELEASE"], False),
    ("Engine compatibility tests", ["-DTESTING=1"], True),
]:
    source = subprocess.check_output(base + flags + ["src/pokemon.c"], cwd=ROOT, text=True)
    routes = re.findall(r"\{\s*FORM_CHANGE_BATTLE_MEGA_EVOLUTION_(ITEM|MOVE)\s*,", source)
    forms = re.findall(r"\.isMegaEvolution\s*=\s*1\b", source)
    if enabled:
        assert {"ITEM", "MOVE"} <= set(routes) and forms, f"{label}: missing compatibility data"
    else:
        assert not routes and not forms, f"{label}: Mega Evolution remains enabled"
    print(f"{label}: {len(routes)} Mega routes, {len(forms)} Mega species — PASS")
