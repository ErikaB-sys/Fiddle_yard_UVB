# Zusatztask "OTA Upload" fuer die bestehende esp32dev-Umgebung.
# Keine zweite Firmware-Build-Umgebung: dieselbe firmware.bin wie fuer USB.
Import("env")

import subprocess
import sys
from pathlib import Path


def run_fy_ota(target, source, env):
    # SCons Action-Callbacks erhalten keyword arguments: target, source, env.
    host = env.GetProjectOption("custom_ota_host", "").strip()
    project = Path(env.subst("$PROJECT_DIR"))
    firmware = Path(env.subst("$BUILD_DIR")) / (
        env.subst("${PROGNAME}") + ".bin"
    )
    command = [
        sys.executable,
        "-u",
        str(project / "scripts" / "fy_ota_upload.py"),
        "--host", host,
        "--file", str(firmware),
    ]
    result = subprocess.call(command, cwd=str(project))
    if result != 0:
        raise SystemExit(result)


# Dieses Script wird nur vom normalen esp32dev-Environment geladen.
# Den Custom-Task deshalb immer registrieren, auch bei PlatformIO-Integration-Dumps.
# Kein optionaler PIOENV-Guard: andernfalls ist 'ota' im SCons-Targetgraph unsichtbar.
env.AddCustomTarget(
    name="ota",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=run_fy_ota,
    title="OTA Upload",
    description=(
        "Kompiliert bei Bedarf und uebertraegt firmware.bin per WLAN "
        "nach 3 Sekunden physischer STOP-Freigabe"
    ),
)
