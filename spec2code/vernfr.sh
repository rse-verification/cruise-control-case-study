#!/usr/bin/env bash
# Adapter for Spec2Code's VerNFR critic. Inputs are staged in one attempt folder.
set -euo pipefail

folder=""
modname=""
main="cruise_control"
checks=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --folder|--modname|--main|--checks)
            [[ $# -ge 2 ]] || { echo "Missing value for $1" >&2; exit 2; }
            case "$1" in
                --folder) folder="$2" ;;
                --modname) modname="$2" ;;
                --main) main="$2" ;;
                --checks) checks="$2" ;;
            esac
            shift 2
            ;;
        *) echo "Unknown option: $1" >&2; exit 2 ;;
    esac
done
[[ -n "$folder" && -n "$modname" ]] || {
    echo "Expected --folder, --modname, and --checks" >&2; exit 2;
}
[[ "$checks" == "control-flow" || "$checks" == "data-flow" ]] || {
    echo "Expected --checks control-flow or data-flow" >&2; exit 2;
}
for extension in c h is; do
    [[ -f "$folder/$modname.$extension" ]] || {
        echo "File not found: $folder/$modname.$extension" >&2; exit 2;
    }
done

run_nfr() {
    local text status
    if text=$(frama-c "$@" 2>&1); then
        printf '%s\n' "$text"
    else
        status=$?
        printf '%s\n' "$text"
        return "$status"
    fi
    # VerNFR may report a violation as a warning while exiting successfully.
    if grep -Eq '^\[nfr[^]]*\].*(Warning|Error):' <<< "$text"; then
        echo "ERROR: VerNFR reported a violation." >&2
        return 1
    fi
}

common=(-vernfr -nfr-ispec "$folder/$modname.is"
        "-cpp-extra-args=-I$folder" -main "$main" -keep-unused-functions all)
if [[ "$checks" == "control-flow" ]]; then
    echo "Checking rule control-flow"
    if grep -nE '#[[:space:]]*include[[:space:]]*["<][^">]+\.c[">]' "$folder/$modname.c"; then
        echo "ERROR: The implementation must include headers, not C source files." >&2
        exit 1
    fi
    run_nfr "${common[@]}" -nfr-check-calls -nfr-fun-ptrs \
        -nfr-all-entries-defined -nfr-only-entries "$folder/$modname.c"
    run_nfr "${common[@]}" -keep-unused-types -nfr-all-entries-declared \
        -nfr-no-fun-defs "$folder/$modname.h"
else
    echo "Checking rule data-flow"
    run_nfr "${common[@]}" -nfr-static-vars -nfr-proper-init \
        -nfr-check-ptr-literals "$folder/$modname.c"
fi
