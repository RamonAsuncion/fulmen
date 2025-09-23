# fulmen

A minimal container runtime and tooling.

Quick start

Platform support: native runtime only supports Linux hosts. I don't have any plans to support macOS or Windows as of now.

- Run a local image tarball:

```bash
./cli/fulmen run /path/to/image.tar
```

- Pull and run an image reference:

```bash
./cli/fulmen run example-image:latest
```

Optional flags

- `-c` / `--command` : command string to run inside the container filesystem.
- `--keep-tar` : if the CLI fetched a tarball, keep it instead of removing the temporary file.

Build

```bash
# build everything (backend + cli + ui)
make setup

# build only CLI
make cli

# run UI (locally)
make ui-run

# run CLI without installing
./cli/fulmen run example-image:latest
```
