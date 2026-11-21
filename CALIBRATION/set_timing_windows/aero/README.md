# SHMS AERO

```bash
bash CALIBRATION/set_timing_windows/aero/replay_aero.sh
# Wait until replay completes. 解包完成后运行下面的独立画图命令。
bash CALIBRATION/set_timing_windows/aero/plot_aero.sh
bash CALIBRATION/set_timing_windows/aero/plot_combined_aero.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
先改脚本顶部的run、事件数、raw类型及输入/输出路径。输出已存在时replay会停止，保留旧数据。
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
普通脚本直接解包，不提交Slurm、不自动画图。Python plotting needs pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
修改解包输出目录后，画图的INPUT_DIR也需对应修改。Plots/raw/replayed ROOTs are not committed.

- `code/replay_aero_lad.C`: SHMS no-timing replay.
- `code/aero_output.def`: exports P.aero.*, hodoscope start/offset and T.shms.*.
- `code/aero_windows_lad.C`: seven PMTs per side, mult>0 and |diffTime|<1000.
- Plot range [-450,450] ns; no detailed PDF. Production paero_cuts.param supplies lines.
- Replay/plot defaults use no_timing_windows_refcut_offset200/aero_inputs.

Edit run lists, input/output directories and plot settings at the top of the scripts.
绘图cut来自PARAM_RUN选择的production参数。Combined uses the same drawing code as individual runs.
