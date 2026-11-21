#!/usr/bin/env bash
# ================= USER SETTINGS / 用户设置 =================
RUN_ID="${RUN_ID:-23360}"
RUN_IDS="${RUN_IDS:-22072 22138 22205 22261 22306 22419 22465 22634 22694 22852 22858 23010 23039 23360 23696 23742}"
RUN_LIST_FILE="${RUN_LIST_FILE:-}" # Optional text file; overrides RUN_IDS. One ID per line; # comments allowed.
EVENTS="${EVENTS:--1}" # Existing ROOT filename label; plotting does not replay events.
PARAM_RUN="${PARAM_RUN:-}" # Empty: RUN_ID, or first combined run.
OUTPUT_TAG="${OUTPUT_TAG:-}" # Empty: RUN_ID or combined.
WORKFLOW_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="${REPO_DIR:-$(cd "$WORKFLOW_DIR/../../.." && pwd)}"
INPUT_DIR="${INPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/no_timing_windows_refcut_offset200}"
HMS_INPUT_PATTERN="${HMS_INPUT_PATTERN:-"hms_LADCOIN_noTimingWindows_{run}_{events}.root"}"
SHMS_INPUT_PATTERN="${SHMS_INPUT_PATTERN:-"shms_noTimingWindows_{run}_{events}.root"}"
OUTPUT_DIR="${OUTPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/no_timing_windows_refcut_offset200/plots}"
DETAILED="${DETAILED:-1}"
HODO_X_MIN="${HODO_X_MIN:--250}"
HODO_X_MAX="${HODO_X_MAX:-150}"
CAL_X_MIN="${CAL_X_MIN:--500}"
CAL_X_MAX="${CAL_X_MAX:-500}"
CER_X_MIN="${CER_X_MIN:--800}"
CER_X_MAX="${CER_X_MAX:-800}"
DETAIL_HALF_WIDTH="${DETAIL_HALF_WIDTH:-40}"
HCER_DETAIL_MIN="${HCER_DETAIL_MIN:--250}"
HCER_DETAIL_MAX="${HCER_DETAIL_MAX:-350}"
PCER_DETAIL_MIN="${PCER_DETAIL_MIN:--400}"
PCER_DETAIL_MAX="${PCER_DETAIL_MAX:-200}"
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
export RUN_ID RUN_IDS RUN_LIST_FILE EVENTS PARAM_RUN OUTPUT_TAG WORKFLOW_DIR REPO_DIR INPUT_DIR HMS_INPUT_PATTERN SHMS_INPUT_PATTERN OUTPUT_DIR DETAILED HODO_X_MIN HODO_X_MAX CAL_X_MIN CAL_X_MAX CER_X_MIN CER_X_MAX DETAIL_HALF_WIDTH HCER_DETAIL_MIN HCER_DETAIL_MAX PCER_DETAIL_MIN PCER_DETAIL_MAX PEAK_MIN_ENTRIES PEAK_SEARCH_SIGMA PEAK_SEARCH_THRESHOLD DATABASE PARAM_OVERRIDE HCANA_BIN PYTHON_BIN ENV_SETUP LOG_DIR WORK_DIR
exec "$PYTHON_BIN" "$REPO_DIR/CALIBRATION/set_timing_windows/plot_workflow.py" standard combined
