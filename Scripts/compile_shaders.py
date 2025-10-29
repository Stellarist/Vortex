import subprocess
import os

SLANGC = "slangc"
SHADER_DIR = "Shaders"
OUT_DIR = "Shaders"

os.makedirs(OUT_DIR, exist_ok=True)

for filename in os.listdir(SHADER_DIR):
    if filename.endswith(".slang"):
        src_path = os.path.join(SHADER_DIR, filename)
        name_wo_ext = os.path.splitext(filename)[0]
        out_path = os.path.join(OUT_DIR, f"{name_wo_ext}.spv")
        
        cmd = [SLANGC, src_path, "-o", out_path]
        print("Compiling:", " ".join(cmd))
        result = subprocess.run(cmd)
        if result.returncode != 0:
            print(f"Error compiling {filename}")
            exit(1)

print("All shaders compiled!")
