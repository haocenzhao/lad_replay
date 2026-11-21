# HMS/SHMS TRIG

```bash
bash CALIBRATION/set_timing_windows/trig/replay_trig.sh
# Wait until replay completes. 解包完成后运行下面的独立画图命令。
bash CALIBRATION/set_timing_windows/trig/plot_trig.sh
bash CALIBRATION/set_timing_windows/trig/plot_combined_trig.sh
```

Edit settings at the top of each script: RUN_ID, EVENTS (-1 = all), RAW_DIR,
RAW_PATTERN (Production or Production_noGEM), OUTPUT_DIR, and environment settings.
先改脚本顶部的run、事件数、raw类型及输入/输出路径。输出已存在时replay会停止，保留旧数据。
Load HCANA/ROOT/LADlib first, or set ENV_SETUP.
Replay runs directly and only generates replay ROOT files; it never starts plotting.
普通脚本直接解包，不提交Slurm、不自动画图。Python plotting needs pypdf.
Changing replay OUTPUT_DIR requires changing the matching plotting INPUT_DIR too.
修改解包输出目录后，画图的INPUT_DIR也需对应修改。Plots/raw/replayed ROOTs are not committed.

- `code/replay_spectrometers_lad.C`: spectrometers-only replay copy, retaining normal parameters/DEF.
- `code/plot_trig_timing_windows_lad.C`: all configured HMS/SHMS ADC/TDC channels, mult>0.
- One PDF/ROOT per arm; ADC/TDC on separate pages, up to4x4 panels per page.
- Non-empty axes use selected min/max; empty channels remain visible. No detailed PDF.
- Default input/output directory for replay: spectrometers_only; plots: TRIG_timing_windows.
- TRIG parameter windows are diagnostics; these scripts do not change their cuts.

Edit run lists, input/output directories and plot settings at the top of the scripts.
绘图cut来自PARAM_RUN选择的production参数。Combined uses the same drawing code as individual runs.
