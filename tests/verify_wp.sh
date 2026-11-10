#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Run from the repository root. Requires Frama-C, configured provers, and jq.
set -euo pipefail

if [[ $# -eq 0 ]]; then
    set -- cruise_control.c cruise_control_ver.c
fi
outdir="${WP_OUTPUT_DIR:-build/wp}"
mkdir -p "$outdir"
failed=0

for source in "$@"; do
    name=$(basename "$source" .c)
    log="$outdir/$name.log"
    report="$outdir/$name.json"
    rm -f "$report"
    if ! timeout 240s frama-c -cpp-extra-args=-Iheaders "$source" \
        -wp -wp-rte -wp-model real -wp-status \
        -wp-prover "${WP_PROVERS:-alt-ergo,z3,cvc5}" \
        -wp-timeout "${WP_TIMEOUT:-10}" -wp-cache none \
        -wp-report-json "$report" > "$log" 2>&1; then
        echo "::error::Frama-C failed or timed out for $source. See $log."
        failed=1
        continue
    fi

    # A zero Frama-C exit code does not mean that every obligation was proved.
    counts=$(sed -nE 's/.*Proved goals:[[:space:]]*([0-9]+)[[:space:]]*\/[[:space:]]*([0-9]+).*/\1 \2/p' "$log")
    if [[ ! "$counts" =~ ^([0-9]+)[[:space:]]+([0-9]+)$ ]]; then
        echo "::error::No final proof summary for $source. See $log."
        failed=1
        continue
    fi
    read -r proved total <<< "$counts"
    echo "$source: $proved of $total goals proved"
    if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
        printf '%s: %s of %s goals proved\n\n' "$source" "$proved" "$total" >> "$GITHUB_STEP_SUMMARY"
    fi
    if (( total == 0 || proved != total )) || \
        ! jq -e 'type == "array" and length > 0 and all(.[]; .passed == true)' "$report" > /dev/null; then
        echo "::error::Verification incomplete for $source. See $log and $report."
        if [[ -f "$report" ]]; then
            jq -r '.[] | select(.passed != true) | "\(.function): \(.property) [\(.verdict)]"' "$report" || true
        fi
        failed=1
    fi
done

exit "$failed"
