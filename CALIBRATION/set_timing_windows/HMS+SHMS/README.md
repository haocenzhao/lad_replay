# HMS and SHMS timing windows

```bash
# Run replay, then run the separate plotting script.
bash CALIBRATION/set_timing_windows/HMS+SHMS/replay_timing_windows.sh
bash CALIBRATION/set_timing_windows/HMS+SHMS/plot_timing_windows_lad.sh
bash CALIBRATION/set_timing_windows/HMS+SHMS/plot_combined_timing_windows_lad.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
Replay stops if an output file already exists; choose another output directory to preserve earlier data.
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
These plain replay scripts do not submit Slurm jobs. Python plotting requires pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
Do not commit generated plots, raw data or replay ROOT files.

| Files | Purpose |
|---|---|
| `code/replay_no_timing_windows_{hms,shms}_lad.C` | Separate HMS/SHMS no-timing replay; use existing timing histogram DEFs. |
| `code/timing_window_setup_lad.C` | Hodo, calorimeter, pre-shower/shower and gas CER plots. |
| `code/combine_timing_window_histograms_lad.C` | Sum compatible input histograms before plotting. |
| `../plot_common.h`, `../plot_workflow.py` | Shared canvas/PDF export and plotting driver. |
| `../replay_workflow.py` | Shared direct replay execution and output checks. |
| `../aero/`, `../DC/`, `../trig/` | Separate workflows with their own READMEs. |

Plot settings are at the top of plot scripts: RUN_ID or RUN_IDS/RUN_LIST_FILE,
INPUT_DIR/patterns, OUTPUT_DIR, PARAM_RUN, axis limits and peak-search settings.
RUN_LIST_FILE overrides RUN_IDS; one ID per line, # comments allowed.
Production parameters provide cut lines; plotting never changes parameter files.
Standard/detailed PDFs and one ROOT with original/detailed folders are retained (DETAILED=1).
Combine sums source histograms/data, not PDFs. Combine compatible timing/offset periods.
Original `code/timing_window_setup.C` remains unchanged. No COIN/TRIG narrow cuts are installed.

Environment: compatible HCANA, ROOT and LADlib; Python3 + pypdf.
The standard LADlib installation supplies LADFilteredStreamBuf.h used by DC/TRIG replay.
Record your HCANA/ROOT/LADlib versions with analysis results; they are external dependencies.

Validation environment: ROOT 6.40.04; HCANA `fcef8e23e162`; LADlib `25c475ce0fb7`.
