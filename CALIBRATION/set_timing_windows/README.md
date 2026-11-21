# Timing-window plots / 时间窗口检查图

All scripts run plots directly in the current replay environment; they never submit Slurm or start replay.
所有脚本只画图，不提交Slurm、不自动replay、不修改参数。

## Edit settings first / 先改脚本顶部

Each entry starts with RUN_ID (single), or RUN_IDS / RUN_LIST_FILE (combined), then INPUT_DIR,
input filename patterns, OUTPUT_DIR, EVENTS, PARAM_RUN, database/parameter sources, and plot ranges.
PEAK_MIN_ENTRIES, PEAK_SEARCH_SIGMA and PEAK_SEARCH_THRESHOLD expose the existing peak-search settings.
每个脚本最开头集中设置run、读取/保存路径、文件名规则、参数run和绘图范围。
A RUN_LIST_FILE overrides RUN_IDS; use one run ID per line, with optional # comments.
Filename patterns use {run} and {events}. EVENTS is a filename label, not a new event selection.
PARAM_RUN defaults to the selected run or first combined run; its production parameters supply cut lines.
Choose runs from one compatible timing/offset period when combining. No automatic run exclusions are applied.

## Entry scripts / 运行入口

Run from any directory with bash and the full path to the desired script:

| Check | Single run | Combined runs |
|---|---|---|
| Hodo, calorimeters, gas Cherenkovs | `plot_timing_windows_lad.sh` | `plot_combined_timing_windows_lad.sh` |
| SHMS AERO | `aero/plot_aero.sh` | `aero/plot_combined_aero.sh` |
| HMS/SHMS DC | `DC/plot_DC.sh` | `DC/plot_combined_DC.sh` |
| HMS/SHMS TRIG diagnostics | `trig/plot_trig.sh` | `trig/plot_combined_trig.sh` |

Examples relative to lad_replay:

```bash
bash CALIBRATION/set_timing_windows/plot_timing_windows_lad.sh
bash CALIBRATION/set_timing_windows/aero/plot_aero.sh
bash CALIBRATION/set_timing_windows/DC/plot_combined_DC.sh
bash CALIBRATION/set_timing_windows/trig/plot_trig.sh
```

Settings can also be supplied as environment variables, e.g. RUN_ID=22138 OUTPUT_DIR=/tmp/review bash ... .

## Required ROOT inputs / 应使用哪种replay

| Check | Input |
|---|---|
| Regular detectors | Dedicated **no-timing-windows replay** ROOTs, containing the existing PMT-vs-time TH2 histograms. HMS and SHMS inputs are separate. |
| AERO | **SHMS no-timing-windows replay** ROOT with `P.aero.goodPos/NegAdcTdcDiffTime` and `P.aero.goodPos/NegAdcMult`, seven PMTs per side. Existing `aero_inputs/` files meet this requirement. The ordinary no-timing DEF may omit these branches; export `P.aero.*` when replaying a new run. |
| DC | No-timing DC diagnostic replay ROOT with both `H.dc.*.rawtdc` and `P.dc.*.rawtdc`. For comparison also provide the matching spectrometers-only ROOT. Keep per-wire tzero enabled and use the same event sample. Existing DC `_work/.../DC_replay/` inputs are reused, not copied or deleted. |
| TRIG | Existing spectrometers-only/production ROOT containing `T.hms.*` and `T.shms.*`. These plots diagnose the currently broad windows; they do not install narrower cuts. |

Regular/AERO windows are in ns. DC and TRIG raw axes are in channels.
TRIG uses selected scalar hits/pulses with mult>0, not every hardware hit; silent channels stay visible.
TRIG non-empty axes use the selected min/max, with a floating-point-safe inclusive maximum; a single value uses one bin.
DC fills all stored selected-hit entries in each plane array, with no added multiplicity cut or normalization.
AERO retains its existing mult>0 and |diffTime|<1000 selection, [-450,450] ns display and one peak marker.

## Outputs / 输出

Output is OUTPUT_DIR/<run or OUTPUT_TAG>/, with one PDF and ROOT per spectrometer where applicable.
Regular plots retain standard and detailed PDFs plus original/detailed ROOT directories (DETAILED=1).
AERO has one standard PDF/ROOT pair. TRIG has one pair for HMS and one for SHMS; ADC and TDC never share a page.
DC_MODE is both, no_timing, or comparison; DC writes to OUTPUT_DIR/no_timing_windows/<tag>/ and/or comparison/<tag>/.
Combined scripts sum source histograms/data and redraw using the same plotting code; they do not concatenate PDFs.
Only plot files are published to output directories. Logs go to LOG_DIR, temporary files to WORK_DIR and are removed.
Existing matching outputs are replaced only after a successful drawing/export pass; unrelated files are retained.

## Environment / 环境

Load the normal HCANA/LAD replay environment first, or set ENV_SETUP, HCANA_BIN and PYTHON_BIN at the top.
Python needs pypdf (`python3 -m pip install --user pypdf`). The existing server also has a user-level fallback package cache.
Original upstream `timing_window_setup.C` is retained unchanged. The active macro is `timing_window_setup_lad.C`.
`plot_common.h` and `plot_workflow.py` are shared helpers. AERO directly includes the shared active macro; no dated snapshots are used.
The scripts derive the repository location from their own paths, so a checkout can be moved or uploaded to GitHub.
No data files, logs, vendored Python archives, Slurm studies or run-specific manifests belong in this source folder.
