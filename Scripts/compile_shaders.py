import subprocess
import os
import sys
import glob

SLANGC = "slangc"

def compile_shader(src_path, source_root=".", output_root=None):
    if not os.path.exists(src_path):
        print(f"Error: File not found: {src_path}")
        return False
    
    if not src_path.endswith(".slang"):
        print(f"Error: File must have .slang extension: {src_path}")
        return False
    
    src_dir = os.path.dirname(src_path)
    if not src_dir:
        src_dir = "."
    filename = os.path.basename(src_path)
    name_wo_ext = os.path.splitext(filename)[0]

    if output_root:
        rel_dir = os.path.relpath(src_dir, source_root)
        if rel_dir == ".":
            out_dir = output_root
        else:
            out_dir = os.path.join(output_root, rel_dir)
    else:
        out_dir = src_dir

    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"{name_wo_ext}.spv")
    
    cmd = [SLANGC, src_path, "-o", out_path]
    print("Compiling:", " ".join(cmd))
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print(f"Error compiling {filename}")
        return False
    return True


if __name__ == "__main__":
    if len(sys.argv) > 1:
        output_root = None
        patterns = sys.argv[1:]

        if "--output-dir" in patterns:
            idx = patterns.index("--output-dir")
            if idx + 1 >= len(patterns):
                print("Error: --output-dir requires a path argument")
                exit(1)
            output_root = patterns[idx + 1]
            patterns = patterns[:idx] + patterns[idx + 2:]

        source_root = os.getcwd()
        success = True
        compiled = 0
        
        # Expand wildcards/globs
        all_files = []
        for pattern in patterns:
            matches = glob.glob(pattern, recursive=True)
            if matches:
                all_files.extend(matches)
            else:
                # If no matches, treat as literal filename
                all_files.append(pattern)
        
        if not all_files:
            print("Warning: No shader files found matching the patterns.")
            exit(0)
        
        for file_path in all_files:
            if compile_shader(file_path, source_root, output_root):
                compiled += 1
            else:
                success = False
        
        if success and compiled > 0:
            print(f"Successfully compiled {compiled} shader(s)!")
            exit(0)
        else:
            exit(1)
    else:
        print("Warning: No shader files specified.")
        print("Usage: python compile_shaders.py [--output-dir <path>] <pattern> [pattern2 ...]")
        print("Examples:")
        print("  python compile_shaders.py --output-dir ../Build/Shaders Deferred/*.slang")
        print("  python compile_shaders.py Forward/pbr.slang Deferred/*.slang")
        exit(0)
