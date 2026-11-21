# Timing-window calibration

| Workflow | Directory | Input replay |
|---|---|---|
| Hodoscopes, calorimeters and gas Cherenkovs | [HMS+SHMS](HMS+SHMS/README.md) | Separate HMS/SHMS no-timing replay |
| SHMS aerogel | [aero](aero/README.md) | SHMS no-timing replay with AERO branches |
| HMS/SHMS drift chambers | [DC](DC/README.md) | DC no-timing and production diagnostic replays |
| HMS/SHMS trigger channels | [trig](trig/README.md) | Spectrometers-only replay |

Each directory contains a plain replay-only script, single-run and combined plotting
scripts, a short README, and a code/ folder. Replay does not submit Slurm or start plotting.
Edit run IDs, input/output paths and environment settings at the top of each script.
Read the workflow README for the replay-then-plot commands, relative to lad_replay.

Shared helpers stay here: plot_common.h, plot_workflow.py and replay_workflow.py.
AERO reuses HMS+SHMS/code/timing_window_setup_lad.C for consistent plotting.
Keep all four workflows and the shared helpers together when copying the calibration code.

Requirements: configured HCANA/ROOT/LADlib and Python3; plotting requires pypdf.
Validation environment: ROOT 6.40.04; HCANA fcef8e23e162; LADlib 25c475ce0fb7.
Do not commit raw data, replay ROOTs, generated plots or logs.
