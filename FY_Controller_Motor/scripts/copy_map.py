Import("env")

from pathlib import Path
import shutil


def copy_map(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    map_file = build_dir / "firmware.map"
    reports_dir = Path(env.subst("$PROJECT_DIR")) / "reports"
    reports_dir.mkdir(parents=True, exist_ok=True)

    if map_file.exists():
        shutil.copy2(map_file, reports_dir / "firmware.map")


env.AddPostAction("$BUILD_DIR/firmware.elf", copy_map)
