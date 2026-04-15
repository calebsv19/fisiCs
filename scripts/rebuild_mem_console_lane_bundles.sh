#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
FISICS_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
WORKSPACE_ROOT="$(CDPATH= cd -- "$FISICS_ROOT/.." && pwd)"
MEM_CONSOLE_ROOT="${WORKSPACE_ROOT}/mem_console"
FISICS_BIN="${FISICS_ROOT}/fisics"

OUT_ROOT="${HOME}/Desktop/fisics_program_binaries/mem_console"
CLANG_DIR="${OUT_ROOT}/clang"
FISICS_DIR="${OUT_ROOT}/fisics"
CLANG_APP="${CLANG_DIR}/eCho.app"
FISICS_APP="${FISICS_DIR}/eCho.app"

timestamp() {
    date +"%Y-%m-%dT%H:%M:%S%z"
}

git_rev() {
    local repo="$1"
    git -C "$repo" rev-parse HEAD 2>/dev/null || echo "unknown"
}

write_provenance() {
    local lane_dir="$1"
    local lane_name="$2"
    local compiler_label="$3"
    local build_cmd="$4"
    local app_path="$5"
    local bin_path="${app_path}/Contents/MacOS/mem-console-bin"
    local launcher_path="${app_path}/Contents/MacOS/mem-console-launcher"
    local sha_bin
    local sha_launcher

    sha_bin="$(shasum -a 256 "$bin_path" | awk '{print $1}')"
    sha_launcher="$(shasum -a 256 "$launcher_path" | awk '{print $1}')"

    cat > "${lane_dir}/PROVENANCE.txt" <<EOF
generated_at=$(timestamp)
lane=${lane_name}
compiler=${compiler_label}
build_command=${build_cmd}
workspace_root=${WORKSPACE_ROOT}
fisics_root=${FISICS_ROOT}
mem_console_root=${MEM_CONSOLE_ROOT}
fisiCs_git_rev=$(git_rev "$FISICS_ROOT")
mem_console_git_rev=$(git_rev "$MEM_CONSOLE_ROOT")
app_path=${app_path}
mem_console_bin_sha256=${sha_bin}
launcher_sha256=${sha_launcher}
EOF
}

build_lane() {
    local lane_name="$1"
    local compiler_label="$2"
    local make_cc_arg="$3"
    local lane_dir="$4"
    local lane_app="$5"
    local build_cmd

    mkdir -p "$lane_dir"
    rm -rf "$lane_app"

    if [[ -n "$make_cc_arg" ]]; then
        build_cmd="make -C ${MEM_CONSOLE_ROOT} clean && make -C ${MEM_CONSOLE_ROOT} CC=${make_cc_arg} package-desktop"
        make -C "$MEM_CONSOLE_ROOT" clean
        make -C "$MEM_CONSOLE_ROOT" CC="$make_cc_arg" package-desktop
    else
        build_cmd="make -C ${MEM_CONSOLE_ROOT} clean && make -C ${MEM_CONSOLE_ROOT} package-desktop"
        make -C "$MEM_CONSOLE_ROOT" clean
        make -C "$MEM_CONSOLE_ROOT" package-desktop
    fi

    cp -R "${MEM_CONSOLE_ROOT}/dist/eCho.app" "$lane_app"
    write_provenance "$lane_dir" "$lane_name" "$compiler_label" "$build_cmd" "$lane_app"
}

if [[ ! -x "$FISICS_BIN" ]]; then
    echo "Missing fisics binary: $FISICS_BIN" >&2
    exit 2
fi

mkdir -p "$OUT_ROOT"

echo "=== mem_console lane reset ==="
echo "output_root=$OUT_ROOT"
echo "clang_app=$CLANG_APP"
echo "fisics_app=$FISICS_APP"

build_lane "clang" "clang" "" "$CLANG_DIR" "$CLANG_APP"
build_lane "fisics" "$FISICS_BIN" "$FISICS_BIN" "$FISICS_DIR" "$FISICS_APP"

echo
echo "=== lane hashes ==="
shasum -a 256 \
    "${CLANG_APP}/Contents/MacOS/mem-console-bin" \
    "${FISICS_APP}/Contents/MacOS/mem-console-bin"

echo
echo "Done."
echo "clang provenance: ${CLANG_DIR}/PROVENANCE.txt"
echo "fisics provenance: ${FISICS_DIR}/PROVENANCE.txt"
