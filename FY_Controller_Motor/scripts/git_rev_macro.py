import subprocess
from pathlib import Path

project_dir = Path(__file__).resolve().parent.parent

try:
    revision = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"],
        cwd=project_dir,
        stderr=subprocess.DEVNULL,
    ).decode("utf-8").strip()
except (subprocess.CalledProcessError, FileNotFoundError):
    revision = "unknown"

print("'-DFW_GIT_COMMIT=\\\"%s\\\"'" % revision)
