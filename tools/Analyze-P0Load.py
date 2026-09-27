"""Read-only G3Load CSV analysis; does not certify a game or change its result.

Example: python tools/Analyze-P0Load.py --run-root Saved/P0Runs/load-01 \
    --warmup-seconds 20 --output Saved/P0Runs/load-01-analysis.json
Legacy smoke runs require explicit --host-subdir host/GUID and
--client-subdir client/GUID. Existing output files are never overwritten.
Only selected numeric columns are retained for exact nearest-rank percentiles.
"""

import argparse
import csv
import json


PROFILE_METRICS = (
    "FrameTime", "GameThreadTime", "RenderThreadTime", "GPUTime",
    "LDP0/GameThread/Combat", "LDP0/GameThread/Timeline",
    "LDG3Load/GameThread/ProbeTick",
)
PROFILE_SELECTORS = ("LDG3Load/Phase", "LDG3Load/SustainSeconds")
NETWORK_COUNTERS = (
    "inTotalBytes", "outTotalBytes", "inTotalPackets", "outTotalPackets",
    "inTotalPacketsLost", "outTotalPacketsLost",
)
SAMPLE_METRICS = (
    "processCpuPercent", "processCpuOneCorePercent", "rssBytes",
    "unitActors", "enemyActors", "aliveEnemies",
)


def number(value):
    try:
        parsed = float(value)
        return parsed if parsed == parsed and abs(parsed) != float("inf") else None
    except (TypeError, ValueError):
        return None


def summary(values):
    if not values:
        return {"n": 0, "mean": None, "p95NearestRank": None, "max": None,
                "min": None}
    ordered = sorted(values)
    return {"n": len(values), "mean": sum(values) / len(values),
            "p95NearestRank": ordered[(95 * len(values) + 99) // 100 - 1],
            "max": ordered[-1], "min": ordered[0]}


def slope(points):
    """Least-squares y per x; a trend description, not a leak detector."""
    if len(points) < 2:
        return None
    mean_x = sum(x for x, _ in points) / len(points)
    mean_y = sum(y for _, y in points) / len(points)
    variance = sum((x - mean_x) ** 2 for x, _ in points)
    return (sum((x - mean_x) * (y - mean_y) for x, y in points) / variance
            if variance else None)


def relative_subdir(value):
    value = value.replace("\\", "/")
    if not value or value.startswith("/") or ":" in value:
        raise argparse.ArgumentTypeError("role subdirectory must be relative")
    if any(part in ("", ".", "..") for part in value.split("/")):
        raise argparse.ArgumentTypeError("role subdirectory cannot contain . or ..")
    return value


def read_json(path):
    try:
        with open(path, encoding="utf-8-sig") as source:
            data = json.load(source)
        if not isinstance(data, dict):
            return {"path": path, "readError": "expected a JSON object"}
        return {"path": path, "data": data}
    except (OSError, ValueError) as error:
        return {"path": path, "readError": str(error)}


def analyze_profile(path, warmup):
    result = {"path": path, "filter": "Phase == 1 and SustainSeconds >= warmup",
              "warmupSeconds": warmup, "units": "milliseconds",
              "limits": [
                  "Timings are per frame, including zero-work frames and the frame cap.",
                  "Combat and Timeline are frame aggregates, not individual-step p95.",
                  "Timeline contains Combat; these nested timings must not be added.",
                  "The sample span is last minus first selected SustainSeconds.",
              ]}
    values = {key: [] for key in PROFILE_METRICS}
    invalid = {key: 0 for key in PROFILE_METRICS}
    first = last = None
    selected = ignored = 0
    try:
        with open(path, newline="", encoding="utf-8-sig") as source:
            rows = csv.reader(source)
            header = next(rows, [])
            indices = {key: header.index(key) for key in
                       PROFILE_METRICS + PROFILE_SELECTORS if key in header}
            result["missingColumns"] = [key for key in
                                        PROFILE_METRICS + PROFILE_SELECTORS
                                        if key not in indices]
            if any(key not in indices for key in PROFILE_SELECTORS):
                result["selectionError"] = "Required selectors absent; no unfiltered fallback."
            else:
                for row in rows:
                    def cell(key):
                        index = indices.get(key)
                        return number(row[index]) if index is not None and index < len(row) else None
                    phase, seconds = (cell(key) for key in PROFILE_SELECTORS)
                    if phase is None or seconds is None:
                        ignored += 1  # UE repeated header and metadata footer included.
                        continue
                    if phase != 1 or seconds < warmup:
                        continue
                    selected += 1
                    first = seconds if first is None else min(first, seconds)
                    last = seconds if last is None else max(last, seconds)
                    for key in PROFILE_METRICS:
                        value = cell(key)
                        if value is not None and value >= 0:
                            values[key].append(value)
                        elif key in indices:
                            invalid[key] += 1
    except (OSError, csv.Error) as error:
        result["readError"] = str(error)
    result.update({"measurementAvailable": selected > 0,
                   "selectedFrames": selected, "ignoredNonnumericRows": ignored,
                   "firstSustainSeconds": first, "lastSustainSeconds": last,
                   "observedSpanSeconds": last - first if first is not None else None,
                   "metrics": {key: dict(summary(values[key]),
                                          invalidOrUnavailableCells=invalid[key])
                               for key in PROFILE_METRICS}})
    return result


def new_network_summary(missing):
    return {"missingColumns": missing, "validIntervals": 0, "validSeconds": 0.0,
            "excludedIntervals": {"missingOrNegativeCounter": 0, "counterDecrease": 0,
                                  "nonpositiveTime": 0},
            "deltas": {key: 0 for key in NETWORK_COUNTERS},
            "limits": [
                "Lifetime-counter differences; rates are total delta / valid elapsed seconds.",
                "Zero-traffic intervals count toward time; -1 and reset/wrap intervals do not.",
                "Exclusion counters count rejected pairs or invalid-counter samples; gaps are not bridged.",
                "One continuous connection is assumed: CSV contains no connection identity.",
            ]}


def network_add(network, previous, row, wall):
    counters = [number(row.get(key)) for key in NETWORK_COUNTERS]
    if any(value is None or value < 0 or not value.is_integer() for value in counters):
        network["excludedIntervals"]["missingOrNegativeCounter"] += 1
        return None
    current = (wall, counters)
    if previous is None:
        return current
    elapsed = wall - previous[0]
    if elapsed <= 0:
        network["excludedIntervals"]["nonpositiveTime"] += 1
        return None  # Never bridge a backwards clock into an already counted interval.
    deltas = [int(value - old) for value, old in zip(counters, previous[1])]
    if any(delta < 0 for delta in deltas):
        network["excludedIntervals"]["counterDecrease"] += 1
        return current
    network["validIntervals"] += 1
    network["validSeconds"] += elapsed
    for key, delta in zip(NETWORK_COUNTERS, deltas):
        network["deltas"][key] += delta
    return current


def analyze_samples(path, warmup):
    result = {"path": path, "filter": "phase == 1 and wallSeconds >= estimated start + warmup",
              "limits": [
                  "Wall-time samples and CSV frame SustainSeconds are not the same clock boundary.",
                  "CPU is OS process percentage; RSS is working set including engine/allocator caches.",
                  "Samples are periodic observations, not continuous integrity or leak proof.",
              ]}
    starts, checkpoints, batches, duplicate_batches = [], [], {}, []
    first_phase = first_full = None
    all_rss_min = all_rss_max = None
    header = []
    try:
        with open(path, newline="", encoding="utf-8-sig") as source:
            rows = csv.DictReader(source)
            header = rows.fieldnames or []
            for row in rows:
                wall, rss = number(row.get("wallSeconds")), number(row.get("rssBytes"))
                phase = number(row.get("phase"))
                label = row.get("label", "")
                if wall is None:
                    continue
                if phase == 1:
                    first_phase = wall if first_phase is None else min(first_phase, wall)
                    if all(number(row.get(key)) == expected for key, expected in
                           (("unitActors", 40), ("enemyActors", 101), ("aliveEnemies", 101))):
                        first_full = wall if first_full is None else min(first_full, wall)
                    if label == "sustain-start":
                        starts.append(wall)
                if rss is None or rss < 0:
                    continue
                all_rss_min = rss if all_rss_min is None else min(all_rss_min, rss)
                all_rss_max = rss if all_rss_max is None else max(all_rss_max, rss)
                if label and label != "periodic":
                    checkpoint = {"label": label, "wallSeconds": wall, "rssBytes": int(rss)}
                    checkpoints.append(checkpoint)
                    parts = label.split("-")
                    if (len(parts) == 4 and parts[0] == "batch" and parts[1].isdigit()
                            and parts[2:] == ["after", "gc"]):
                        batch = int(parts[1])
                        if 1 <= batch <= 25:
                            if batch in batches:
                                duplicate_batches.append(batch)
                            else:
                                batches[batch] = dict(checkpoint, batch=batch)
    except (OSError, csv.Error) as error:
        result["readError"] = str(error)

    if starts:
        start = min(starts)
        method = "explicit sustain-start label"
        uncertainty = "Sample callback time differs from profile frame boundaries."
    elif first_full is not None:
        start = first_full
        method = "fallback: first phase-1 sample with actual 40/101/101 population"
        uncertainty = "True sustain start may precede this sample; window is conservatively shorter."
    else:
        start = first_phase
        method = "fallback: first phase-1 sample (population not confirmed)"
        uncertainty = "Sustain start and representative workload cannot be established from labels."
    result["warmup"] = {"seconds": warmup, "startWallSeconds": start, "method": method,
                        "uncertainty": uncertainty, "sustainStartLabelCount": len(starts),
                        "cutoffWallSeconds": start + warmup if start is not None else None}
    values = {key: [] for key in SAMPLE_METRICS}
    missing = [key for key in SAMPLE_METRICS + NETWORK_COUNTERS +
               ("wallSeconds", "phase", "label") if key not in header]
    result["missingColumns"] = missing
    network = new_network_summary([key for key in NETWORK_COUNTERS if key not in header])
    previous = None
    first = last = None
    selected = 0
    if start is not None and "readError" not in result:
        try:
            with open(path, newline="", encoding="utf-8-sig") as source:
                for row in csv.DictReader(source):
                    wall = number(row.get("wallSeconds"))
                    if (wall is None or number(row.get("phase")) != 1
                            or wall < start + warmup):
                        previous = None
                        continue
                    selected += 1
                    first = wall if first is None else min(first, wall)
                    last = wall if last is None else max(last, wall)
                    for key in SAMPLE_METRICS:
                        value = number(row.get(key))
                        if value is not None and value >= 0:
                            values[key].append(value)
                    if not network["missingColumns"]:
                        previous = network_add(network, previous, row, wall)
        except (OSError, csv.Error) as error:
            result["readError"] = str(error)
    seconds = network["validSeconds"]
    network.update({"available": seconds > 0,
                    "inBytesPerSecond": network["deltas"]["inTotalBytes"] / seconds if seconds else None,
                    "outBytesPerSecond": network["deltas"]["outTotalBytes"] / seconds if seconds else None})
    recent = [batches[key] for key in sorted(batches)[-10:]]
    recent_rss = [row["rssBytes"] for row in recent]
    trend = {"expectedBatches": 25, "observedBatches": len(batches),
             "missingBatchIds": [batch for batch in range(1, 26) if batch not in batches],
             "duplicateBatchIds": duplicate_batches, "recentBatchIds": [r["batch"] for r in recent],
             "recentRssBytes": summary(recent_rss),
             "recentSlopeBytesPerBatch": slope([(r["batch"], r["rssBytes"]) for r in recent]),
             "recentSlopeBytesPerSecond": slope([(r["wallSeconds"], r["rssBytes"]) for r in recent]),
             "leakAssessment": "Deferred: working-set slope alone cannot prove or disprove a leak.",
             "limits": "Only explicit batch-1..25-after-gc labels count; batch 0 is sustain teardown.",
             "recentWindowComplete": len(recent) == 10}
    result.update({"selectedSamples": selected, "firstWallSeconds": first,
                   "lastWallSeconds": last,
                   "observedSpanSeconds": last - first if first is not None else None,
                   "metrics": {key: summary(values[key]) for key in SAMPLE_METRICS},
                   "network": network,
                   "rss": {"allSampleMinBytes": all_rss_min, "allSampleMaxBytes": all_rss_max,
                           "checkpoints": checkpoints, "postGcTrend": trend}})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run-root", required=True)
    parser.add_argument("--warmup-seconds", type=float, default=20.0)
    parser.add_argument("--output", required=True, help="New JSON path; parent directory must exist")
    parser.add_argument("--host-subdir", type=relative_subdir, default="host")
    parser.add_argument("--client-subdir", type=relative_subdir, default="client")
    args = parser.parse_args()
    if number(args.warmup_seconds) is None or args.warmup_seconds < 0:
        parser.error("--warmup-seconds must be finite and nonnegative")
    root = args.run_root.rstrip("/\\")
    report = {"schemaVersion": 1, "analysisKind": "Read-only G3Load fixture analysis",
              "runRoot": args.run_root, "warmupSeconds": args.warmup_seconds,
              "verificationResult": "NotAssessed",
              "verificationPolicy": "This analysis never promotes an execution result to Pass.",
              "sourceRunner": read_json(root + "/pair.json"), "roles": {}}
    failed_sources = []
    if str(report["sourceRunner"].get("data", {}).get("Result", "")).lower() == "fail":
        failed_sources.append("pair.json")
    for role, subdir in (("host", args.host_subdir), ("client", args.client_subdir)):
        directory = root + "/" + subdir
        proof = read_json(directory + "/result.json")
        if str(proof.get("data", {}).get("result", "")).lower() == "fail":
            failed_sources.append(subdir + "/result.json")
        report["roles"][role] = {
            "directory": directory, "sourceFixtureProof": proof,
            "profile": analyze_profile(directory + "/profile.csv", args.warmup_seconds),
            "samples": analyze_samples(directory + "/samples.csv", args.warmup_seconds),
        }
    report["failedExecutionSources"] = failed_sources
    if failed_sources:
        report["verificationResult"] = "Fail (preserved from source execution)"
    try:
        with open(args.output, "x", encoding="utf-8", newline="\n") as destination:
            json.dump(report, destination, indent=2, ensure_ascii=False, allow_nan=False)
            destination.write("\n")
    except FileExistsError:
        parser.error("--output already exists; refusing to overwrite: " + args.output)
    except OSError as error:
        parser.error("cannot create --output: " + str(error))
    print(json.dumps({"output": args.output, "verificationResult": report["verificationResult"],
                      "selectedFrames": {role: data["profile"]["selectedFrames"]
                                         for role, data in report["roles"].items()}}))


if __name__ == "__main__":
    main()
