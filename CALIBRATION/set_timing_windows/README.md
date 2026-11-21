# Timing windows / 时间窗口

```bash
# Run replay, then run the separate plotting script.
bash CALIBRATION/set_timing_windows/replay_timing_windows.sh
bash CALIBRATION/set_timing_windows/plot_timing_windows_lad.sh
bash CALIBRATION/set_timing_windows/plot_combined_timing_windows_lad.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
先改脚本顶部的run、事件数、raw类型及输入/输出路径。输出已存在时replay会停止，保留旧数据。
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
普通脚本直接解包，不提交Slurm、不自动画图。Python plotting needs pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
修改解包输出目录后，画图的INPUT_DIR也需对应修改。Plots/raw/replayed ROOTs are not committed.

| Files | Purpose / 用途 |
|---|---|
| `replay_code/replay_no_timing_windows_{hms,shms}_lad.C` | Separate HMS/SHMS no-timing replay; use existing timing histogram DEFs. |
| `timing_window_setup_lad.C` | Hodo, calorimeter, pre-shower/shower and gas CER plots. |
| `combine_timing_window_histograms_lad.C` | Sum compatible input histograms before plotting. |
| `plot_common.h`, `plot_workflow.py` | Shared canvas/PDF export and plotting driver. |
| `replay_workflow.py` | Shared direct replay execution and output checks. |
| `aero/`, `DC/`, `trig/` | Dedicated replay/plot scripts, code and short READMEs. |

Plot settings are at the top of plot scripts: RUN_ID or RUN_IDS/RUN_LIST_FILE,
INPUT_DIR/patterns, OUTPUT_DIR, PARAM_RUN, axis limits and peak-search settings.
RUN_LIST_FILE overrides RUN_IDS; one ID per line, # comments allowed.
Production parameters provide cut lines; plotting never changes parameter files.
Standard/detailed PDFs and one ROOT with original/detailed folders are retained (DETAILED=1).
Combine sums source histograms/data, not PDFs. Combine compatible timing/offset periods.
Original `timing_window_setup.C` remains unchanged. No COIN/TRIG narrow cuts are installed.

Environment: compatible HCANA, ROOT and LADlib; Python3 + pypdf.
The standard LADlib installation supplies LADFilteredStreamBuf.h used by DC/TRIG replay.
Record your HCANA/ROOT/LADlib versions with analysis results; they are external dependencies.

Validation environment: ROOT 6.40.04; HCANA `fcef8e23e162`; LADlib `25c475ce0fb7`.
