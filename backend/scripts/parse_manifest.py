#!/usr/bin/env python3
import sys
import json

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'manifest.json'
    try:
        with open(path, 'r') as f:
            m = json.load(f)
    except Exception as e:
        sys.exit(1)

    if not isinstance(m, list) or not m:
        return

    first = m[0]
    layers = first.get('Layers') or first.get('layers') or []
    for l in layers:
        print(l)

if __name__ == '__main__':
    main()
