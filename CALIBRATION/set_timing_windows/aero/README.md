# SHMS AERO

```bash
bash CALIBRATION/set_timing_windows/aero/replay_aero.sh
# After replay completes, run the separate plotting commands below.
bash CALIBRATION/set_timing_windows/aero/plot_aero.sh
bash CALIBRATION/set_timing_windows/aero/plot_combined_aero.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
Replay stops if an output file already exists; choose another output directory to preserve earlier data.
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
These plain replay scripts do not submit Slurm jobs. Python plotting requires pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
Do not commit generated plots, raw data or replay ROOT files.

- `code/replay_aero_lad.C`: SHMS no-timing replay.
- `code/aero_output.def`: exports P.aero.*, hodoscope start/offset and T.shms.*.
- `code/aero_windows_lad.C`: seven PMTs per side, mult>0 and |diffTime|<1000.
- Plot range [-450,450] ns; no detailed PDF. Production paero_cuts.param supplies lines.
- Replay/plot defaults use no_timing_windows_refcut_offset200/aero_inputs.

Edit run lists, input/output directories and plot settings at the top of the scripts.
Cut lines use the production parameters selected by PARAM_RUN. Combined plots use the same drawing code as individual runs.
