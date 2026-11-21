# Reference time / 参考时间

```bash
# Commands below are relative to lad_replay. 先运行replay，完成后单独画图。
bash CALIBRATION/set_reftimes/replay_reference_time.sh
bash CALIBRATION/set_reftimes/plot_reference_time.sh
bash CALIBRATION/set_reftimes/plot_combined_reference_time.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
先改脚本顶部的run、事件数、raw类型及输入/输出路径。输出已存在时replay会停止，保留旧数据。
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
普通脚本直接解包，不提交Slurm、不自动画图。Python plotting needs pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
修改解包输出目录后，画图的INPUT_DIR也需对应修改。Plots/raw/replayed ROOTs are not committed.

- `code/replay_reference_events_hms.C`, `...shms.C`: no-ref replay with wide downstream timing windows.
- `code/hms_reference_events.def`, `shms_reference_events.def`: save `T.hms.*` / `T.shms.*`, including EDTM and hDCREF1–5/pDCREF1–10.
- `reference_time_setup_lad.C`: multiplicity, fixed/detailed ref-time views and EDTM overlays.
- `combine_reference_time_histograms_lad.C`: sum saved per-run plot histograms; run single plots first.
- `plot_*.sh`: edit RUN_ID or RUN_IDS/RUN_LIST_FILE, input patterns, ranges and output paths at the top.

Blue: mult>0. Red: selected mult1/2/3 categories (REF_MULT_RATIO=10).
Green dashed: matching EDTM monitor mult>0 and raw time>0; drawn last.
Cut lines use production parameters selected by PARAM_RUN (default: run/first combined run).
EVENTS in plot scripts is a filename label. Detailed width defaults to about1200 channels.
Original `reference_time_setup.C` is retained. Shared helpers live in ../set_timing_windows.
