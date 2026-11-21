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
INPUT_DIR="${INPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/DC_timing_windows/_work/no_timing/DC_replay}"
INPUT_PATTERN="${INPUT_PATTERN:-"NO_TIMING_DC_{run}_{events}.root"}"
PRODUCTION_DIR="${PRODUCTION_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/DC_timing_windows/_work/production/DC_replay}"
PRODUCTION_PATTERN="${PRODUCTION_PATTERN:-"SPEC_ONLY_DC_{run}_{events}.root"}"
OUTPUT_DIR="${OUTPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/DC_timing_windows}"
DC_MODE="${DC_MODE:-both}" # both | no_timing | comparison
DETAILED="${DETAILED:-1}"
DC_X_MIN="${DC_X_MIN:--16000}"
DC_X_MAX="${DC_X_MAX:--2000}"
DC_DETAIL_MARGIN="${DC_DETAIL_MARGIN:-600}"
DC_STORAGE_MIN="${DC_STORAGE_MIN:--30000}"
DC_STORAGE_MAX="${DC_STORAGE_MAX:-30000}"
DC_BIN_WIDTH="${DC_BIN_WIDTH:-5}"
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
export RUN_ID RUN_IDS RUN_LIST_FILE EVENTS PARAM_RUN OUTPUT_TAG WORKFLOW_DIR REPO_DIR INPUT_DIR INPUT_PATTERN PRODUCTION_DIR PRODUCTION_PATTERN OUTPUT_DIR DC_MODE DETAILED DC_X_MIN DC_X_MAX DC_DETAIL_MARGIN DC_STORAGE_MIN DC_STORAGE_MAX DC_BIN_WIDTH DATABASE PARAM_OVERRIDE HCANA_BIN PYTHON_BIN ENV_SETUP LOG_DIR WORK_DIR
exec "$PYTHON_BIN" "$REPO_DIR/CALIBRATION/set_timing_windows/plot_workflow.py" dc combined
