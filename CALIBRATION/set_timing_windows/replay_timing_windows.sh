#!/usr/bin/env bash
# ================= USER SETTINGS / 用户设置 =================
RUN_ID="${RUN_ID:-23360}"
EVENTS="${EVENTS:--1}"
RUN_TYPE="${RUN_TYPE:-0}" # Production=0; RAW_PATTERN below determines the input file.
RAW_DIR="${RAW_DIR:-/ceph/submit/data/group/lad/raw}"
RAW_PATTERN="${RAW_PATTERN:-lad_Production_%05d.dat.0}" # For noGEM: lad_Production_noGEM_%05d.dat.0
OUTPUT_DIR="${OUTPUT_DIR:-/ceph/submit/data/group/lad/spectrometers_replay/no_timing_windows_refcut_offset200}"
WORKFLOW_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="${REPO_DIR:-$(cd "$WORKFLOW_DIR/../.." && pwd)}"
LOG_DIR="${LOG_DIR:-$OUTPUT_DIR/replay_logs}" # Replay logs/reports, outside the source folders.
HCANA_BIN="${HCANA_BIN:-hcana}"
PYTHON_BIN="${PYTHON_BIN:-python3}"
ENV_SETUP="${ENV_SETUP:-}" # Optional environment setup shell file.
# ============================================================
set -eo pipefail
if [[ -n "$ENV_SETUP" ]]; then source "$ENV_SETUP"; fi
set -u
export RUN_ID EVENTS RUN_TYPE RAW_DIR RAW_PATTERN OUTPUT_DIR REPO_DIR LOG_DIR HCANA_BIN PYTHON_BIN ENV_SETUP
exec "$PYTHON_BIN" "$REPO_DIR/CALIBRATION/set_timing_windows/replay_workflow.py" standard
