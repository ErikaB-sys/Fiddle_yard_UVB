import subprocess
from pathlib import Path

project_dir = Path(__file__).resolve().parent.parent

try:
    revision = subprocess.check_output(
        ["git", "rev-parse", "--short", "HEAD"],
        cwd=project_dir,
        stderr=subprocess.DEVNULL,
    ).decode("utf-8").strip()

    status = subprocess.check_output(
        ["git", "status", "--porcelain"],
        cwd=project_dir,
        stderr=subprocess.DEVNULL,
    ).decode("utf-8").strip()

    if status:
        revision += "-dirty"
except (subprocess.CalledProcessError, FileNotFoundError):
    revision = "unknown"

print("-DFW_GIT_COMMIT=\\\"%s\\\"" % revision)
