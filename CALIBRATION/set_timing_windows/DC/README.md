# HMS/SHMS DC

```bash
bash CALIBRATION/set_timing_windows/DC/replay_DC.sh
# Wait until replay completes. 解包完成后运行下面的独立画图命令。
bash CALIBRATION/set_timing_windows/DC/plot_DC.sh
bash CALIBRATION/set_timing_windows/DC/plot_combined_DC.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
先改脚本顶部的run、事件数、raw类型及输入/输出路径。输出已存在时replay会停止，保留旧数据。
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
普通脚本直接解包，不提交Slurm、不自动画图。Python plotting needs pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
修改解包输出目录后，画图的INPUT_DIR也需对应修改。Plots/raw/replayed ROOTs are not committed.

- `code/replay_dc_lad.C`: shared replay; DC_MODE=both/no_timing/production.
  Production uses normal parameters; no_timing additionally loads the two DC wide-cut files.
- `code/dc_tree.def`: exports rawtdc/time/nhit/RefTime arrays for both spectrometers.
- `code/dc_timing_windows_lad.C`: selected-hit plots per plane; both arms have12 planes.
- Keep h_using_tzero_per_wire=p_using_tzero_per_wire=1.
- Replay NO_TIMING_DIR/PRODUCTION_DIR must match plot INPUT_DIR/PRODUCTION_DIR.
- Plot DC_MODE=both/no_timing/comparison; blue=production, orange dashed=no-timing
  in comparisons; red dashed=production cuts. No extra multiplicity cut is added.
- Output folders: no_timing_windows/<run or tag> and comparison/<run or tag>.

Edit run lists, input/output directories and plot settings at the top of the scripts.
绘图cut来自PARAM_RUN选择的production参数。Combined uses the same drawing code as individual runs.
