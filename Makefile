PREFIX ?= /usr/local
CLI_BIN = cli/fulmen
BACKEND_BUILD = backend/build
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

.PHONY: all setup backend cli ui ui-run clean rebuild install uninstall

all: setup

setup: backend cli ui

backend:
	cmake -S backend -B $(BACKEND_BUILD)
	cmake --build $(BACKEND_BUILD) -- -j$(JOBS)

cli:
	$(MAKE) -C cli

ui:
	@if command -v dotnet >/dev/null 2>&1; then \
		dotnet build ui/ui.csproj -c Debug; \
		LIB=`ls $(BACKEND_BUILD)/libfulmen_backend.* 2>/dev/null | head -n1 || true`; \
		if [ -n "$$LIB" ]; then \
			UI_BIN=`find ui/bin -type d -path "*/net*" -print -quit || true`; \
			if [ -n "$$UI_BIN" ]; then cp -v "$$LIB" "$$UI_BIN/"; fi; \
		fi; \
	else \
		printf 'dotnet not found; skipping UI build\n' >&2; \
	fi

ui-run:
	@if command -v dotnet >/dev/null 2>&1; then \
		cd ui && dotnet run --no-build -c Debug; \
	else \
		printf 'dotnet not found\n' >&2; \
	fi

clean:
	-@rm -rf $(BACKEND_BUILD)
	-@$(MAKE) -C cli clean || true
	-@if [ -d ui ]; then dotnet clean ui/ui.csproj -c Debug || true; fi

rebuild: clean setup

install: setup
	sudo mkdir -p $(PREFIX)/bin $(PREFIX)/lib
	sudo cp $(CLI_BIN) $(PREFIX)/bin/
	@if [ -f $(BACKEND_BUILD)/libfulmen_backend.so ]; then \
		sudo cp $(BACKEND_BUILD)/libfulmen_backend.so $(PREFIX)/lib/ && sudo ldconfig || true; \
		fi
	@if [ -f $(BACKEND_BUILD)/libfulmen_backend.dylib ]; then \
		sudo cp $(BACKEND_BUILD)/libfulmen_backend.dylib $(PREFIX)/lib/; \
		fi

uninstall:
	sudo rm -f $(PREFIX)/bin/fulmen
	sudo rm -f $(PREFIX)/lib/libfulmen_backend.so
	sudo rm -f $(PREFIX)/lib/libfulmen_backend.dylib
