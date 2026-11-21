# Reference-time plots / Reference time画图

## Run / 运行

```bash
bash CALIBRATION/set_reftimes/plot_reference_time.sh
bash CALIBRATION/set_reftimes/plot_combined_reference_time.sh
```

The scripts can be launched from any directory with their full path. All editable settings are at the top:
RUN_ID or RUN_IDS/RUN_LIST_FILE, INPUT_DIR, HMS/SHMS_INPUT_PATTERN, OUTPUT_DIR, EVENTS, PARAM_RUN,
fixed axis maxima, REF_DETAIL_WIDTH, REF_MULT_RATIO, database/parameter overrides, executables and log/work paths.
脚本顶部集中设置run、输入/输出路径、参数来源和绘图范围。RUN_LIST_FILE覆盖RUN_IDS，每行一个run，支持#注释。

## Replay inputs / 输入

Single-run plots require **no-reference-time replay**, with wide downstream timing windows and complete reference branches,
including hDCREF1-5 / pDCREF1-10 and the corresponding EDTM monitor branches.
Default patterns select the existing `*_noReferenceTime_refEvents_*` files. Change the patterns for another compatible replay.
Missing branches fail explicitly; they are not silently drawn as empty channels.
These are selected reference hits and recorded multiplicities, not all raw TDC hardware hits.

Combined plots read the saved **per-run reference plot ROOT files**, retaining the histograms/, mult categories and EDTM subsets.
Run the single-run script for any new run first. Change INPUT_DIR and filename patterns to locate those plot files.
The combined script sums full saved histograms, not PDFs or truncated visible ranges. It then redraws with the same macro.
PARAM_RUN defaults to the first combined run and selects the production cut overlays; combine only compatible run periods.

## Output and selection / 输出与筛选

One HMS PDF/ROOT pair and one SHMS PDF/ROOT pair are written under OUTPUT_DIR/<run or OUTPUT_TAG>/.
Each reference has one page: multiplicity (including zero), fixed-range time and detailed time.
Blue time histograms require mult>0. The red overlay sums sufficiently populated categories among mult1/2/3,
using REF_MULT_RATIO (default10). Green dashed overlays tag the matching arm's EDTM monitor mult>0 and rawtime>0,
and are drawn last. This is an EDTM-monitor tag, not an independent proof of a hardware trigger type.
The detailed width defaults to about1200 channels, expanding if needed to include resolved ordinary/EDTM peaks.
EVENTS is only the existing filename event-count label; no replay is performed and no events are newly discarded by that setting.

Original `reference_time_setup.C` is retained unchanged. Active code is `reference_time_setup_lad.C` plus the combined histogram helper.
Shared PDF/driver helpers live in ../set_timing_windows; move the complete CALIBRATION folder together.
Use a configured HCANA environment and Python with pypdf. ENV_SETUP is optional; logs and scratch are outside the repository.
The user-level package cache already on the existing server is only a fallback; other machines can install pypdf normally.
