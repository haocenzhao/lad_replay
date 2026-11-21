#!/usr/bin/env bash
# ================= USER SETTINGS / 用户设置 =================
RUN_ID="${RUN_ID:-23360}"
EVENTS="${EVENTS:--1}" # Existing ROOT filename label; plotting does not replay events.
PARAM_RUN="${PARAM_RUN:-}" # Empty: RUN_ID, or first combined run.
OUTPUT_TAG="${OUTPUT_TAG:-}" # Empty: RUN_ID or combined.
WORKFLOW_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="${REPO_DIR:-$(cd "$WORKFLOW_DIR/../.." && pwd)}"
INPUT_DIR="${INPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/no_ref_time_loosecuts_v1}"
HMS_INPUT_PATTERN="${HMS_INPUT_PATTERN:-"hms_noReferenceTime_refEvents_{run}_{events}.root"}"
SHMS_INPUT_PATTERN="${SHMS_INPUT_PATTERN:-"shms_noReferenceTime_refEvents_{run}_{events}.root"}"
OUTPUT_DIR="${OUTPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/no_ref_time_loosecuts_v1/plots}"
REF_HMS_T_MAX="${REF_HMS_T_MAX:-50000}"
REF_SHMS_T_MAX="${REF_SHMS_T_MAX:-30000}"
REF_ROC_MAX="${REF_ROC_MAX:-10000}"
REF_DC_MAX="${REF_DC_MAX:-30000}"
REF_DETAIL_WIDTH="${REF_DETAIL_WIDTH:-1200}"
REF_MULT_RATIO="${REF_MULT_RATIO:-10}"
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
export RUN_ID EVENTS PARAM_RUN OUTPUT_TAG WORKFLOW_DIR REPO_DIR INPUT_DIR HMS_INPUT_PATTERN SHMS_INPUT_PATTERN OUTPUT_DIR REF_HMS_T_MAX REF_SHMS_T_MAX REF_ROC_MAX REF_DC_MAX REF_DETAIL_WIDTH REF_MULT_RATIO DATABASE PARAM_OVERRIDE HCANA_BIN PYTHON_BIN ENV_SETUP LOG_DIR WORK_DIR
exec "$PYTHON_BIN" "$REPO_DIR/CALIBRATION/set_timing_windows/plot_workflow.py" ref single
