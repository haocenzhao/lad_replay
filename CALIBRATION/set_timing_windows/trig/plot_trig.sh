#!/usr/bin/env bash
# ================= USER SETTINGS / 用户设置 =================
RUN_ID="${RUN_ID:-23360}"
EVENTS="${EVENTS:--1}" # Existing ROOT filename label; plotting does not replay events.
PARAM_RUN="${PARAM_RUN:-}" # Empty: RUN_ID, or first combined run.
OUTPUT_TAG="${OUTPUT_TAG:-}" # Empty: RUN_ID or combined.
WORKFLOW_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="${REPO_DIR:-$(cd "$WORKFLOW_DIR/../../.." && pwd)}"
INPUT_DIR="${INPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/spectrometers_only}"
INPUT_PATTERN="${INPUT_PATTERN:-"SPEC_ONLY_{run}_{events}.root"}"
OUTPUT_DIR="${OUTPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/TRIG_timing_windows}"
TRIG_PARAM_hms="${TRIG_PARAM_hms:-PARAM/TRIG/thms.param}"
TRIG_PARAM_shms="${TRIG_PARAM_shms:-PARAM/TRIG/tshms.param}"
ADC_BIN_WIDTH="${ADC_BIN_WIDTH:-2}"
TDC_BIN_WIDTH="${TDC_BIN_WIDTH:-5}"
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
export RUN_ID EVENTS PARAM_RUN OUTPUT_TAG WORKFLOW_DIR REPO_DIR INPUT_DIR INPUT_PATTERN OUTPUT_DIR TRIG_PARAM_hms TRIG_PARAM_shms ADC_BIN_WIDTH TDC_BIN_WIDTH DATABASE PARAM_OVERRIDE HCANA_BIN PYTHON_BIN ENV_SETUP LOG_DIR WORK_DIR
exec "$PYTHON_BIN" "$REPO_DIR/CALIBRATION/set_timing_windows/plot_workflow.py" trig single
