#!/usr/bin/env python3
import subprocess
import sys

config = "Release" if len(sys.argv) > 1 and sys.argv[1] == "release" else "Debug"
backend_build = "backend/build"
ui_dir = "ui"

subprocess.run(["cmake", "-S", "backend", "-B", backend_build], check=True)
subprocess.run(["cmake", "--build", backend_build, "--config", config], check=True)
subprocess.run(["dotnet", "build", ui_dir, "-c", config], check=True)

