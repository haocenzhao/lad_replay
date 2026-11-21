#!/usr/bin/env bash
# ================= USER SETTINGS / 用户设置 =================
RUN_ID="${RUN_ID:-23360}"
EVENTS="${EVENTS:--1}" # Existing ROOT filename label; plotting does not replay events.
PARAM_RUN="${PARAM_RUN:-}" # Empty: RUN_ID, or first combined run.
OUTPUT_TAG="${OUTPUT_TAG:-}" # Empty: RUN_ID or combined.
WORKFLOW_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="${REPO_DIR:-$(cd "$WORKFLOW_DIR/../../.." && pwd)}"
INPUT_DIR="${INPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/no_timing_windows_refcut_offset200/aero_inputs}"
INPUT_PATTERN="${INPUT_PATTERN:-"SHMS_aero_noTimingWindows_{run}_{events}.root"}"
OUTPUT_DIR="${OUTPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/AERO_timing_windows}"
AERO_PARAM_FILE="${AERO_PARAM_FILE:-PARAM/SHMS/AERO/paero_cuts.param}"
AERO_X_MIN="${AERO_X_MIN:--450}"
AERO_X_MAX="${AERO_X_MAX:-450}"
DETAILED=0 # Keep the current AERO workflow: one standard PDF.
PEAK_MIN_ENTRIES="${PEAK_MIN_ENTRIES:-1000}"
PEAK_SEARCH_SIGMA="${PEAK_SEARCH_SIGMA:-1.0}"
PEAK_SEARCH_THRESHOLD="${PEAK_SEARCH_THRESHOLD:-0.001}"
DATABASE="${DATABASE:-DBASE/LAD_COIN/standard.database}"
PARAM_OVERRIDE="${PARAM_OVERRIDE:-}" # Optional additional parameter file; no parameter files are written.
HCANA_BIN="${HCANA_BIN:-hcana}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
ENV_SETUP="${ENV_SETUP:-}" # Optional shell file to source; normally leave empty in a configured replay environment.
LOG_DIR="${LOG_DIR:-/tmp/lad_plot_logs}"
WORK_DIR="${WORK_DIR:-/tmp}"
# ============================================================
set -eo pipefail
if [[ -n "$ENV_SETUP" ]]; then source "$ENV_SETUP"; fi
set -u
export RUN_ID EVENTS PARAM_RUN OUTPUT_TAG WORKFLOW_DIR REPO_DIR INPUT_DIR INPUT_PATTERN OUTPUT_DIR AERO_PARAM_FILE AERO_X_MIN AERO_X_MAX DETAILED PEAK_MIN_ENTRIES PEAK_SEARCH_SIGMA PEAK_SEARCH_THRESHOLD DATABASE PARAM_OVERRIDE HCANA_BIN PYTHON_BIN ENV_SETUP LOG_DIR WORK_DIR
exec "$PYTHON_BIN" "$REPO_DIR/CALIBRATION/set_timing_windows/plot_workflow.py" aero single
