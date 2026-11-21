# HMS/SHMS DC

```bash
bash CALIBRATION/set_timing_windows/DC/replay_DC.sh
# After replay completes, run the separate plotting commands below.
bash CALIBRATION/set_timing_windows/DC/plot_DC.sh
bash CALIBRATION/set_timing_windows/DC/plot_combined_DC.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
Replay stops if an output file already exists; choose another output directory to preserve earlier data.
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
These plain replay scripts do not submit Slurm jobs. Python plotting requires pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
Do not commit generated plots, raw data or replay ROOT files.

- `code/replay_dc_lad.C`: shared replay; DC_MODE=both/no_timing/production.
  Production uses normal parameters; no_timing additionally loads the two DC wide-cut files.
- `code/dc_tree.def`: exports rawtdc/time/nhit/RefTime arrays for both spectrometers.
- `code/dc_timing_windows_lad.C`: selected-hit plots per plane; both arms have 12 planes.
- Keep h_using_tzero_per_wire=p_using_tzero_per_wire=1.
- Replay NO_TIMING_DIR/PRODUCTION_DIR must match plot INPUT_DIR/PRODUCTION_DIR.
- Plot DC_MODE=both/no_timing/comparison; blue=production, orange dashed=no-timing
  in comparisons; red dashed=production cuts. No extra multiplicity cut is added.
- Output folders: no_timing_windows/<run or tag> and comparison/<run or tag>.

Edit run lists, input/output directories and plot settings at the top of the scripts.
Cut lines use the production parameters selected by PARAM_RUN. Combined plots use the same drawing code as individual runs.
