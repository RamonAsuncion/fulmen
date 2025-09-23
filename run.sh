#!/usr/bin/env bash
set -e
./build.py
cd ui
dotnet run --no-build -c Debug
