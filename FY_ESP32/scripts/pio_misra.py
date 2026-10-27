# Separate PlatformIO target: pio run -t misra
# Bewusst noch kein Pflicht-Gate fuer jeden Build; siehe Issue #163.
Import("env")
import subprocess
import sys
from pathlib import Path

def run_misra(source, target, env):
    root = Path(env.subst("$PROJECT_DIR"))
    script = root / "scripts" / "misra_check.py"
    result = subprocess.call([sys.executable, str(script)], cwd=str(root))
    if result:
        raise SystemExit(result)

env.AddCustomTarget(
    name="misra",
    dependencies=None,
    actions=run_misra,
    title="MISRA / Cppcheck",
    description="Separater MISRA-orientierter Prueflauf (Cppcheck erforderlich)"
)
