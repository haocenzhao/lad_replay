# Reference time

```bash
# Run these commands from lad_replay. Replay first, then plot separately.
bash CALIBRATION/set_reftimes/replay_reference_time.sh
bash CALIBRATION/set_reftimes/plot_reference_time.sh
bash CALIBRATION/set_reftimes/plot_combined_reference_time.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
Replay stops if an output file already exists; choose another output directory to preserve earlier data.
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
These plain replay scripts do not submit Slurm jobs. Python plotting requires pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
Do not commit generated plots, raw data or replay ROOT files.

- `code/replay_reference_events_hms.C`, `...shms.C`: no-ref replay with wide downstream timing windows.
- `code/hms_reference_events.def`, `shms_reference_events.def`: save `T.hms.*` / `T.shms.*`, including EDTM and hDCREF1-5/pDCREF1-10.
- `reference_time_setup_lad.C`: multiplicity, fixed/detailed ref-time views and EDTM overlays.
- `combine_reference_time_histograms_lad.C`: sum saved per-run plot histograms; run single plots first.
- `plot_*.sh`: edit RUN_ID or RUN_IDS/RUN_LIST_FILE, input patterns, ranges and output paths at the top.

Blue: mult>0. Red: selected mult1/2/3 categories (REF_MULT_RATIO=10).
Green dashed: matching EDTM monitor mult>0 and raw time>0; drawn last.
Cut lines use production parameters selected by PARAM_RUN (default: run/first combined run).
EVENTS in plot scripts is a filename label. Detailed width defaults to about 1200 channels.
Original `reference_time_setup.C` is retained. Shared helpers live in ../set_timing_windows.
