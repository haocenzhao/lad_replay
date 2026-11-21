#!/usr/bin/env python3
"""Portable sequential plotting driver. User settings belong in the shell scripts."""

from pathlib import Path
import os
import sys
import re
import json
import subprocess
import tempfile
import shutil


def merge_pdf(destination, pages):
    try:
        from pypdf import PdfReader, PdfWriter
        from pypdf.generic import RectangleObject
    except ImportError:
        cached = Path.home() / ".local/share/lad_plot_python/pypdf_runtime.zip"
        if cached.is_file():
            sys.path.insert(0, str(cached))
        try:
            from pypdf import PdfReader, PdfWriter
            from pypdf.generic import RectangleObject
        except ImportError as exc:
            raise SystemExit(
                "PDF export requires pypdf: python3 -m pip install --user pypdf"
            ) from exc
    writer = PdfWriter()
    expected = 0
    for name in pages:
        reader = PdfReader(name)
        expected += len(reader.pages)
        for page in reader.pages:
            # ROOT's paper CropBox contains the canvas, while its MediaBox may
            # retain A4 dimensions. Match them to avoid clipping or blank space.
            page.mediabox = RectangleObject(list(page.cropbox))
        writer.append(reader)
    temporary = Path(str(destination) + ".partial")
    with temporary.open("wb") as stream:
        writer.write(stream)
    if not expected or len(PdfReader(temporary).pages) != expected:
        raise RuntimeError("Incomplete PDF export")
    temporary.replace(destination)


if len(sys.argv) > 1 and sys.argv[1] == "merge":
    merge_pdf(sys.argv[2], sys.argv[3:])
    sys.exit(0)


def cpp(value):
    return json.dumps(str(value), ensure_ascii=True)


def vector(paths):
    return "{" + ",".join(cpp(x) for x in paths) + "}"


def inputs(pattern, directory, runs, events):
    files = [Path(directory) / pattern.format(run=run, events=events) for run in runs]
    for path in files:
        if not path.is_file() or path.stat().st_size == 0:
            raise RuntimeError(f"Missing input ROOT: {path}")
    return files


def main():
    workflow, mode = sys.argv[1:3]
    if workflow not in {"ref", "standard", "aero", "dc", "trig"} or mode not in {
        "single",
        "combined",
    }:
        raise ValueError("Invalid plotting mode")
    env = os.environ.copy()
    repo = Path(env["REPO_DIR"]).resolve()
    timing = repo / "CALIBRATION/set_timing_windows"
    refs = repo / "CALIBRATION/set_reftimes"
    if mode == "single":
        runs = [int(env["RUN_ID"])]
    else:
        text = (
            Path(env["RUN_LIST_FILE"]).read_text()
            if env.get("RUN_LIST_FILE")
            else env.get("RUN_IDS", "")
        )
        text = "\n".join(x.split("#", 1)[0] for x in text.splitlines())
        runs = [int(x) for x in re.split(r"[\s,]+", text.strip()) if x]
    if not runs or any(x <= 0 for x in runs) or len(set(runs)) != len(runs):
        raise ValueError("Run IDs must be positive and unique")
    events = int(env.get("EVENTS", "-1"))
    parameter_run = int(env.get("PARAM_RUN") or runs[0])
    tag = env.get("OUTPUT_TAG") or (str(runs[0]) if mode == "single" else "combined")
    if tag in {".", ".."} or not re.fullmatch(r"[A-Za-z0-9_.-]+", tag):
        raise ValueError("OUTPUT_TAG must be a simple filename component")
    output = Path(env["OUTPUT_DIR"]).expanduser().resolve()
    logs = Path(env.get("LOG_DIR", "/tmp/lad_plot_logs")).expanduser()
    logs.mkdir(parents=True, exist_ok=True)
    label = f"run {runs[0]}" if mode == "single" else f"combined {len(runs)} runs"
    if workflow == "dc" and env.get("DC_MODE", "both") not in {
        "both",
        "no_timing",
        "comparison",
    }:
        raise ValueError("DC_MODE must be both, no_timing or comparison")
    env.update(
        PLOT_REPO=str(repo), PLOT_DRIVER=str(Path(__file__).resolve()), PLOT_LABEL=label
    )
    env["PLOT_DATABASE"] = env.get("DATABASE", "DBASE/LAD_COIN/standard.database")
    executable = env.get("HCANA_BIN", "hcana")
    if not shutil.which(executable):
        raise RuntimeError(
            f"HCANA not found: {executable}; load the replay environment or set HCANA_BIN"
        )
    staged = []
    with tempfile.TemporaryDirectory(
        prefix="lad_plots_", dir=env.get("WORK_DIR") or None
    ) as scratch:
        scratch = Path(scratch)
        env["PLOT_SCRATCH"] = str(scratch)

        def execute(includes, body, name):
            path = scratch / "task.C"
            path.write_text(
                "\n".join("#include " + cpp(x) for x in includes)
                + "\n#include <TSystem.h>\nvoid task(){try{\n"
                + body
                + '\nstd::cout<<"LAD_PLOT_OK"<<std::endl;\n}catch(const std::exception &e){std::cerr<<e.what()<<std::endl;gSystem->Exit(1);}}\n'
            )
            log = logs / f"{workflow}_{mode}_{tag}_{name}.log"
            with log.open("w") as stream:
                result = subprocess.run(
                    [executable, "-l", "-b", "-q", str(path)],
                    cwd=repo,
                    env=env,
                    stdout=stream,
                    stderr=subprocess.STDOUT,
                )
            text = log.read_text()
            if result.returncode or "LAD_PLOT_OK" not in text:
                raise RuntimeError(f"Plotting failed; see {log}\n{text[-3500:]}")

        def add_outputs(root):
            files = [root, root.with_suffix(".pdf")]
            detail = root.with_name(root.stem + "_detailed.pdf")
            if detail.is_file():
                files.append(detail)
            for file in files:
                if not file.is_file() or not file.stat().st_size:
                    raise RuntimeError(f"Missing plot output: {file}")
                staged.append(file)

        if workflow in {"ref", "standard"}:
            for arm in ("HMS", "SHMS"):
                data = inputs(
                    env[arm + "_INPUT_PATTERN"], env["INPUT_DIR"], runs, events
                )
                source = data[0]
                filename = (
                    f'{arm}_{"reference_time_lad" if workflow=="ref" else "timing_windows"}_{tag}'
                    + (f"_{events}" if mode == "single" else "")
                    + ".root"
                )
                root = scratch / filename
                includes = (
                    [refs / "reference_time_setup_lad.C"]
                    if workflow == "ref"
                    else [timing / "timing_window_setup_lad.C"]
                )
                body = ""
                if mode == "combined":
                    source = scratch / f"{arm}_summed_histograms.root"
                    if workflow == "ref":
                        includes.append(
                            refs / "combine_reference_time_histograms_lad.C"
                        )
                        body += f'combine_reference_inputs_lad({vector(data)},{cpp(arm.lower())},{cpp(source)},{{{",".join(map(str,runs))}}});\n'
                    else:
                        includes.append(
                            timing / "combine_timing_window_histograms_lad.C"
                        )
                        body += f'if(!combine_timing_window_histograms_lad({vector(data)},{arm.lower()}_timing_window_histogram_names_lad(),{cpp(source)}))throw std::runtime_error("Histogram combination failed");\n'
                if workflow == "ref":
                    body += f"run_{arm.lower()}_reference_time_setup({cpp(source)},{parameter_run},{cpp(root)});"
                else:
                    body += f'{{TFile fresh({cpp(root)},"RECREATE");}}\nrun_{arm.lower()}_timing_windows({cpp(source)},{cpp(root)},{parameter_run});\nexport_timing_windows_pdf({cpp(root)},{cpp(root.with_suffix(".pdf"))});'
                execute(includes, body, arm)
                add_outputs(root)
        elif workflow == "aero":
            data = inputs(env["INPUT_PATTERN"], env["INPUT_DIR"], runs, events)
            root = scratch / f"SHMS_timingwindow_aero_{tag}.root"
            execute(
                [timing / "aero/code/aero_windows_lad.C"],
                f"run_aero_plots_lad({vector(data)},{parameter_run},{cpp(root)},{cpp(label)});",
                "SHMS",
            )
            add_outputs(root)
        elif workflow == "trig":
            data = inputs(env["INPUT_PATTERN"], env["INPUT_DIR"], runs, events)
            for arm in ("HMS", "SHMS"):
                root = scratch / f"{arm}_TRIG_timing_windows_{tag}.root"
                execute(
                    [timing / "trig/code/plot_trig_timing_windows_lad.C"],
                    f"plot_trig_timing_windows_lad({vector(data)},{cpp(arm.lower())},{parameter_run},{cpp(root)});",
                    arm,
                )
                add_outputs(root)
        else:
            no_timing = inputs(env["INPUT_PATTERN"], env["INPUT_DIR"], runs, events)
            production = (
                []
                if env.get("DC_MODE") == "no_timing"
                else inputs(
                    env["PRODUCTION_PATTERN"], env["PRODUCTION_DIR"], runs, events
                )
            )
            execute(
                [timing / "DC/code/dc_timing_windows_lad.C"],
                f'run_dc_plots_lad({vector(no_timing)},{vector(production)},{parameter_run},{cpp(str(scratch)+"/")},{cpp(tag)});',
                "DC",
            )
            for prefix in ("DC_noTiming_", "DC_comparison_"):
                root = scratch / f"{prefix}{tag}.root"
                if root.is_file():
                    add_outputs(root)
        destinations = []
        for file in staged:
            directory = output / tag
            if workflow == "dc":
                directory = (
                    output
                    / (
                        "comparison"
                        if file.name.startswith("DC_comparison_")
                        else "no_timing_windows"
                    )
                    / tag
                )
            directory.mkdir(parents=True, exist_ok=True)
            target = directory / file.name
            pending = target.with_name(target.name + ".partial")
            shutil.copy2(file, pending)
            pending.replace(target)
            destinations.append(target)
        print(f"{workflow}: runs={runs}, parameter_run={parameter_run}")
        for path in destinations:
            print(path)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        raise SystemExit(str(error))
