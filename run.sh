#!/usr/bin/env bash
# Build (unless -n/--no-build) and run an Arda executable from the repo root.
#
# Examples:
#   ./run.sh                  # Debug, mac preset
#   ./run.sh release          # Release, mac preset
#   ./run.sh -p mac -c release
#   ./run.sh -n                # just launch what's already built
set -euo pipefail

config="Debug"
preset="mac"
target="arda_scene"
no_build=0

# First positional arg may be a bare config name, e.g. `./run.sh release`.
if [[ $# -gt 0 && "$1" != -* ]]; then
    config="$1"
    shift
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--config) config="$2"; shift 2 ;;
        -p|--preset) preset="$2"; shift 2 ;;
        -t|--target) target="$2"; shift 2 ;;
        -n|--no-build) no_build=1; shift ;;
        --) shift; break ;;
        *) break ;;
    esac
done

case "$(echo "$config" | tr '[:upper:]' '[:lower:]')" in
    debug) config="Debug" ;;
    release) config="Release" ;;
    relwithdebinfo) config="RelWithDebInfo" ;;
    minsizerel) config="MinSizeRel" ;;
esac

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_dir"

binary_dir="build/$preset"
if [[ ! -d "$binary_dir" ]]; then
    echo "Not configured. Run:  cmake --preset $preset" >&2
    exit 1
fi

if [[ "$no_build" -eq 0 ]]; then
    cmake --build "$binary_dir" --config "$config" --target "$target"
fi

exe="$binary_dir/bin/$config/$target"
if [[ ! -x "$exe" ]]; then
    echo "Not found: $exe" >&2
    exit 1
fi

exec "$exe" "$@"
