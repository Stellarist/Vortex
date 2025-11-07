import sys
import shutil
from pathlib import Path

RESOURCES = [
    "Assets",
    "Configs",
    "Scripts",
    "Shaders/*.spv",
]

def copy_resources(src_dir, build_dir):
    src = Path(src_dir)
    build = Path(build_dir)
    
    for resource in RESOURCES:
        if "*" in resource:
            parts = resource.split("/")
            dir_name = parts[0]
            pattern = parts[1] if len(parts) > 1 else "*"
            
            src_path = src / dir_name
            dst_path = build / dir_name
            dst_path.mkdir(parents=True, exist_ok=True)
            
            matched_files = list(src_path.rglob(pattern))
            for file in matched_files:
                if file.is_file():
                    relative_path = file.relative_to(src_path)
                    dest_file = dst_path / relative_path
                    dest_file.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(file, dest_file)
                    print(f"Copied {dir_name}/{relative_path}")
        else:
            src_path = src / resource
            dst_path = build / resource
            
            if src_path.exists():
                shutil.copytree(src_path, dst_path, dirs_exist_ok=True)
                print(f"Copied {resource}/")
            else:
                print(f"Warning: {resource} does not exist")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: copy_resources.py <source_dir> <build_dir>")
        sys.exit(1)
    
    copy_resources(sys.argv[1], sys.argv[2])
