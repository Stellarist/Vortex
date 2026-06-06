import sys
import shutil
from pathlib import Path

RESOURCES = ["Assets", "Configs", "Scripts"]

def copy_resources(src_dir, build_dir):
    src = Path(src_dir)
    build = Path(build_dir)
    has_error = False

    for resource in RESOURCES:
        try:
            if "*" in resource:
                parts = resource.split("/")
                dir_name = parts[0]
                pattern = parts[1] if len(parts) > 1 else "*"

                src_path = src / dir_name
                dst_path = build / dir_name

                if not src_path.exists():
                    print(f"Warning: {resource} does not exist")
                    has_error = True
                    continue

                matched_files = list(src_path.rglob(pattern))

                if not matched_files:
                    print(f"Warning: no files matched {resource}")
                    has_error = True
                    continue

                dst_path.mkdir(parents=True, exist_ok=True)

                for file in matched_files:
                    if file.is_file():
                        relative_path = file.relative_to(src_path)
                        dest_file = dst_path / relative_path
                        dest_file.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(file, dest_file)

            else:
                src_path = src / resource
                dst_path = build / resource

                if src_path.exists():
                    shutil.copytree(src_path, dst_path, dirs_exist_ok=True)
                else:
                    print(f"Warning: {resource} does not exist")
                    has_error = True

        except Exception as e:
            print(f"Error: failed to copy {resource}: {e}")
            has_error = True

    if not has_error:
        print("Success: resources copied successfully")

    return not has_error


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: copy_resources.py <source_dir> <build_dir>")
        sys.exit(1)

    success = copy_resources(sys.argv[1], sys.argv[2])
    sys.exit(0 if success else 1)
