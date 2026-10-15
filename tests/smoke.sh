#!/bin/bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TEMP="$(mktemp -d)"
trap 'rm -f -- "$TEMP/probe.so" "$TEMP/fault.so" "$TEMP/invalid.shader" "$TEMP/link.shader" "$TEMP/output"; rmdir -- "$TEMP"' EXIT

bash "$ROOT/build.sh"
g++ -std=c++17 -Wall -Wextra -O2 -o "$ROOT/tests/glx_smoke" \
    "$ROOT/tests/glx_smoke.cpp" -lGL -lX11 -ldl -pthread
g++ -std=c++17 -Wall -Wextra -fPIC -shared -o "$TEMP/probe.so" \
    "$ROOT/tests/swap_probe.cpp" -lGL -ldl
g++ -std=c++17 -Wall -Wextra -O2 -fPIC -shared -Wl,--wrap=dlsym -o "$TEMP/fault.so" \
    "$ROOT/lib.cpp" "$ROOT/tests/symbol_failure.cpp" -lGL -ldl -pthread
printf 'this is not GLSL\n' > "$TEMP/invalid.shader"
printf '#version 330 compatibility\nvoid notMain() {}\n' > "$TEMP/link.shader"

run_check() {
    local mode="$1" shader="$2" library="$3" diagnostic="$4"
    if ! env OGL_INJECT_SHADER="$shader" LD_PRELOAD="$library:$TEMP/probe.so" \
        "$ROOT/tests/glx_smoke" "$mode" > "$TEMP/output" 2>&1; then
        cat "$TEMP/output"
        return 1
    fi
    cat "$TEMP/output"
    grep -Fq "$diagnostic" "$TEMP/output"
}

run_check injected "$ROOT/fragment.shader" "$ROOT/ogl_inject.so" "Shader program ready"
run_check fallback "$TEMP/missing.shader" "$ROOT/ogl_inject.so" "Cannot open shader"
run_check fallback "$TEMP/invalid.shader" "$ROOT/ogl_inject.so" "compilation failed"
run_check fallback "$TEMP/link.shader" "$ROOT/ogl_inject.so" "creation/link failed"
run_check fallback fragment.shader "$ROOT/ogl_inject.so" "must be an absolute path"
OGL_TEST_MISSING_SYMBOL=glCreateShader \
    run_check fallback "$ROOT/fragment.shader" "$TEMP/fault.so" "Cannot resolve glCreateShader"
OGL_TEST_MISSING_SYMBOL=glBindSampler \
    run_check fallback "$ROOT/fragment.shader" "$TEMP/fault.so" "Cannot resolve glBindSampler"
OGL_TEST_MISSING_SYMBOL=glXSwapBuffers \
    run_check no-swap "$ROOT/fragment.shader" "$TEMP/fault.so" "cannot forward swap"
run_check contexts "$ROOT/fragment.shader" "$ROOT/ogl_inject.so" "Skipping additional context"
run_check framebuffer "$ROOT/fragment.shader" "$ROOT/ogl_inject.so" "Skipping unsupported framebuffer"
run_check core "$ROOT/fragment.shader" "$ROOT/ogl_inject.so" "Core-profile contexts are not supported"

# No shader override: resolve fragment.shader beside the library, not from cwd.
env -u OGL_INJECT_SHADER LD_PRELOAD="$ROOT/ogl_inject.so:$TEMP/probe.so" \
    "$ROOT/tests/glx_smoke" injected

echo "All GLX smoke checks passed."
