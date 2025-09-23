PREFIX ?= $(shell if [ `id -u` -eq 0 ]; then echo "/usr/local"; else echo "$(HOME)/local"; fi)
CLI_BIN = cli/fulmen
BACKEND_BUILD = backend/build
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

.PHONY: all setup backend cli ui ui-run clean rebuild install uninstall

all: setup

setup: backend cli ui

backend:
	cmake -S backend -B $(BACKEND_BUILD)
	cmake --build $(BACKEND_BUILD) -- -j$(JOBS)

cli: backend
	$(MAKE) -C cli

ui: backend
	@if ! command -v dotnet >/dev/null 2>&1; then \
		printf 'dotnet not found, skipping UI build...\n' >&2; \
		exit 1; \
	fi
	dotnet build ui/ui.csproj -c Debug || exit 1
	LIB=`ls $(BACKEND_BUILD)/libfulmen_backend.* 2>/dev/null | head -n1 || true`
	if [ -n "$$LIB" ]; then \
		UI_BIN=`find ui/bin -type d -path "*/net*" -print -quit || true`; \
		if [ -n "$$UI_BIN" ]; then \
			cp -v "$$LIB" "$$UI_BIN/" || exit 1; \
		else \
			printf 'UI build output directory not found...\n' >&2; exit 1; \
		fi; \
	else \
		printf 'Backend library not found, skipping...\n' >&2; exit 1; \
	fi

ui-run:
	@if command -v dotnet >/dev/null 2>&1; then \
		cd ui && dotnet run --no-build -c Debug; \
		else \
		printf 'dotnet not found!\n' >&2; \
		fi

clean:
	-@rm -rf $(BACKEND_BUILD)
	-@$(MAKE) -C cli clean || true
	-@if [ -d ui ]; then \
		dotnet clean ui/ui.csproj -c Debug || true; \
		rm -rf ui/obj ui/bin; \
		fi

rebuild: clean setup

install: setup
	mkdir -p $(PREFIX)/bin $(PREFIX)/lib
	cp $(CLI_BIN) $(PREFIX)/bin/
	@if [ -f $(BACKEND_BUILD)/libfulmen_backend.so ]; then \
		cp $(BACKEND_BUILD)/libfulmen_backend.so $(PREFIX)/lib/; \
		if [ "$(PREFIX)" = "/usr/local" ]; then ldconfig; fi; \
	fi
	@if [ -f $(BACKEND_BUILD)/libfulmen_backend.dylib ]; then \
		cp $(BACKEND_BUILD)/libfulmen_backend.dylib $(PREFIX)/lib/; \
	fi

uninstall:
	rm -f $(PREFIX)/bin/fulmen
	rm -f $(PREFIX)/lib/libfulmen_backend.so
	rm -f $(PREFIX)/lib/libfulmen_backend.dylib
