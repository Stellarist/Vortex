import re
import subprocess
import sys
from pathlib import Path

SLANGC = "slangc"
ENTRY_POINT_PATTERN = re.compile(
    r'\[\s*shader\s*\(\s*"(vertex|pixel|compute|geometry)"\s*\)\s*\]'
    r'\s*(?:\[[^\]]*\]\s*)*\w+\s+(\w+)\s*\('
)


def run_slangc(src_path, output_path, *options):
    command = [SLANGC, str(src_path), *options, "-o", str(output_path)]
    result = subprocess.run(command)
    if result.returncode != 0:
        print(f"Error: failed to compile {src_path} -> {output_path}")
        return False
    return True


def compile_shader(src_path, source_root, output_root, entry_points):
    output_path = output_root / src_path.relative_to(source_root)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    if not run_slangc(src_path, output_path.with_suffix(".spv")):
        return False

    for stage, entry in entry_points:
        if not run_slangc(
            src_path,
            output_path.with_suffix(f".{entry}.dxil"),
            "-target", "dxil",
            "-profile", "sm_6_0",
            "-entry", entry,
            "-stage", stage,
        ):
            return False

    return True


if __name__ == "__main__":
    args = sys.argv[1:]
    output_root = Path(".")

    if args:
        if len(args) != 2 or args[0] != "--output-dir":
            print("Usage: python compile_shaders.py [--output-dir <path>]")
            sys.exit(1)

        output_root = Path(args[1])

    source_root = Path.cwd()
    shader_files = sorted(source_root.rglob("*.slang"))

    if not shader_files:
        print("Warning: no shader files found")
        sys.exit(0)

    success = True
    compiled = 0
    skipped = 0

    for src_path in shader_files:
        entry_points = ENTRY_POINT_PATTERN.findall(src_path.read_text(encoding="utf-8"))
        if not entry_points:
            skipped += 1
            continue

        if compile_shader(src_path, source_root, output_root, entry_points):
            compiled += 1
        else:
            success = False

    if success:
        print(f"Success: compiled {compiled} shader(s) successfully, skipped {skipped} include-only file(s)")
        sys.exit(0)

    sys.exit(1)
