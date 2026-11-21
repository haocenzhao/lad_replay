# HMS/SHMS TRIG

```bash
bash CALIBRATION/set_timing_windows/trig/replay_trig.sh
# After replay completes, run the separate plotting commands below.
bash CALIBRATION/set_timing_windows/trig/plot_trig.sh
bash CALIBRATION/set_timing_windows/trig/plot_combined_trig.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
Replay stops if an output file already exists; choose another output directory to preserve earlier data.
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
These plain replay scripts do not submit Slurm jobs. Python plotting requires pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
Do not commit generated plots, raw data or replay ROOT files.

- `code/replay_spectrometers_lad.C`: spectrometers-only replay copy, retaining normal parameters/DEF.
- `code/plot_trig_timing_windows_lad.C`: all configured HMS/SHMS ADC/TDC channels, mult>0.
- One PDF/ROOT per arm; ADC/TDC on separate pages, up to 4x4 panels per page.
- Non-empty axes use selected min/max; empty channels remain visible. No detailed PDF.
- Default input/output directory for replay: spectrometers_only; plots: TRIG_timing_windows.
- TRIG parameter windows are diagnostics; these scripts do not change their cuts.

Edit run lists, input/output directories and plot settings at the top of the scripts.
Cut lines use the production parameters selected by PARAM_RUN. Combined plots use the same drawing code as individual runs.
