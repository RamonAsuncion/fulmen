#!/usr/bin/env python3
import os
import shutil
import subprocess
import sys

config="Release" if len(sys.argv) > 1 and sys.argv[1] == "release" else "Debug"
backend_build="backend/build"
ui_dir="ui"
lib_name="libfulmen_backend.so"

subprocess.run(["cmake", "-S", "backend", "-B", backend_build], check=True)
subprocess.run(["cmake", "--build", backend_build, "--config", config], check=True)
subprocess.run(["dotnet", "build", ui_dir, "-c", config], check=True)

src=os.path.join(backend_build, lib_name)
dst=os.path.join(ui_dir, "bin", "Debug", "net9.0", lib_name)
os.makedirs(os.path.dirname(dst), exist_ok=True)
shutil.copy2(src, dst)
