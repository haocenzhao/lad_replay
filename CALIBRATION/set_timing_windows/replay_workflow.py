#!/usr/bin/env python3
"""Replay launch helper; edit settings in each workflow's replay_*.sh."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    mode = sys.argv[1]
    env = os.environ.copy()
    repo = Path(env["REPO_DIR"]).resolve()
    run, events, run_type = (int(env[k]) for k in ("RUN_ID", "EVENTS", "RUN_TYPE"))
    if run <= 0 or events == 0 or events < -1:
        raise ValueError("RUN_ID must be positive; EVENTS is -1 or positive")
    raw = Path(env["RAW_DIR"]).resolve() / (env["RAW_PATTERN"] % run)
    if not raw.is_file():
        raise FileNotFoundError(f"Missing raw data: {raw}")
    output = Path(env["OUTPUT_DIR"]).resolve()
    logs = Path(env["LOG_DIR"]).resolve()
    logs.mkdir(parents=True, exist_ok=True)
    if not shutil.which(env["HCANA_BIN"]):
        raise RuntimeError("Load the HCANA/ROOT/LADlib environment first")
    ref = "CALIBRATION/set_reftimes/code/"
    timing = "CALIBRATION/set_timing_windows/"
    tasks = []
    if mode == "ref":
        for arm in ("hms", "shms"):
            tasks.append(
                (
                    ref + f"replay_reference_events_{arm}.C",
                    f"replay_reference_events_{arm}",
                    output / f"{arm}_noReferenceTime_refEvents_{run}_{events}.root",
                    "",
                    [
                        f"T.{arm}.{'h' if arm=='hms' else 'p'}EDTM_tdcTimeRaw",
                        f"T.{arm}.{'h' if arm=='hms' else 'p'}DCREF{5 if arm=='hms' else 10}_tdcTimeRaw",
                    ],
                )
            )
    elif mode == "standard":
        for arm in ("hms", "shms"):
            prefix = "hms_LADCOIN" if arm == "hms" else "shms"
            tasks.append(
                (
                    timing + f"replay_code/replay_no_timing_windows_{arm}_lad.C",
                    f"replay_no_timing_windows_{arm}_lad",
                    output / f"{prefix}_noTimingWindows_{run}_{events}.root",
                    "",
                    [
                        f"{'h' if arm=='hms' else 'p'}hodo_1x_good_adctdc_diff_time_vs_pmt_pos"
                    ],
                )
            )
    elif mode == "aero":
        tasks.append(
            (
                timing + "aero/code/replay_aero_lad.C",
                "replay_aero_lad",
                output / f"SHMS_aero_noTimingWindows_{run}_{events}.root",
                "",
                ["P.aero.goodPosAdcTdcDiffTime", "P.aero.goodNegAdcMult"],
            )
        )
    elif mode == "dc":
        dc_mode = env["DC_MODE"]
        if dc_mode not in ("both", "no_timing", "production"):
            raise ValueError("DC_MODE must be both, no_timing or production")
        for kind in ("no_timing", "production"):
            if dc_mode not in ("both", kind):
                continue
            directory = Path(
                env["NO_TIMING_DIR" if kind == "no_timing" else "PRODUCTION_DIR"]
            )
            prefix = "NO_TIMING" if kind == "no_timing" else "SPEC_ONLY"
            tasks.append(
                (
                    timing + "DC/code/replay_dc_lad.C",
                    "replay_dc_lad",
                    directory / f"{prefix}_DC_{run}_{events}.root",
                    kind,
                    ["H.dc.1u1.rawtdc", "P.dc.1u1.rawtdc"],
                )
            )
    elif mode == "trig":
        tasks.append(
            (
                timing + "trig/code/replay_spectrometers_lad.C",
                "replay_spectrometers_lad",
                output / f"SPEC_ONLY_{run}_{events}.root",
                "",
                ["T.hms.hT1_tdcMultiplicity", "T.shms.pT1_tdcMultiplicity"],
            )
        )
    else:
        raise ValueError("Unknown replay workflow")
    # Validate destinations before any replay, to preserve earlier ROOT data.
    for _, _, path, _, _ in tasks:
        if path.exists():
            raise FileExistsError(
                f"Output already exists; select another OUTPUT_DIR: {path}"
            )
    for macro, function, path, kind, required in tasks:
        path = path.resolve()
        path.parent.mkdir(parents=True, exist_ok=True)
        task_env = env.copy()
        # Sources expect a printf pattern; this exact path has no substitution.
        task_env.update(
            REPLAY_OUTPUT_PATTERN=str(path),
            REPLAY_RAW_DIR=str(raw.parent),
            REPLAY_RAW_PATTERN=env["RAW_PATTERN"],
            REPLAY_DC_MODE=kind,
            REPLAY_REPORT_PATTERN=str(
                logs / f"{function}_{kind}_{run}_{events}.report"
            ),
        )
        arguments = f"{run},{events}" + (
            f",{run_type}" if mode in ("dc", "trig") else ""
        )
        checks = []
        for name in required:
            if mode == "standard":
                checks.append(
                    f'if(!f.Get({json.dumps(name)})) throw std::runtime_error("Missing output histogram");'
                )
            else:
                checks.append(
                    f'if(!t->GetLeaf({json.dumps(name)})) throw std::runtime_error("Missing output branch");'
                )
        body = (
            f"#include {json.dumps(str(repo/macro))}\n"
            "#include <TFile.h>\n#include <TTree.h>\n#include <stdexcept>\n"
            f"void task(){{try{{ {function}({arguments}); "
            f'TFile f({json.dumps(str(path))},"READ"); '
            'if(f.IsZombie()||f.TestBit(TFile::kRecovered)) throw std::runtime_error("Invalid replay output"); '
            + (
                ""
                if mode == "standard"
                else 'auto t=dynamic_cast<TTree*>(f.Get("T")); if(!t||!t->GetEntries()) throw std::runtime_error("Empty output tree"); '
            )
            + "".join(checks)
            + 'std::cout<<"LAD_REPLAY_OK"<<std::endl; '
            "}catch(const std::exception &e){std::cerr<<e.what()<<std::endl;gSystem->Exit(1);}}\n"
        )
        with tempfile.TemporaryDirectory(prefix="lad_replay_") as scratch:
            task = Path(scratch) / "task.C"
            task.write_text(body)
            log = logs / f"{function}_{kind}_{run}_{events}.log"
            with log.open("w") as stream:
                result = subprocess.run(
                    [env["HCANA_BIN"], "-l", "-b", "-q", str(task)],
                    cwd=repo,
                    env=task_env,
                    stdout=stream,
                    stderr=subprocess.STDOUT,
                )
            with log.open("rb") as stream:
                stream.seek(max(0, log.stat().st_size - 65536))
                tail = stream.read().decode(errors="replace")
            if result.returncode or "LAD_REPLAY_OK" not in tail:
                raise RuntimeError(
                    f"Replay failed; inspect {log}. A partial ROOT may remain."
                )
        print(path, flush=True)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        raise SystemExit(str(exc))
