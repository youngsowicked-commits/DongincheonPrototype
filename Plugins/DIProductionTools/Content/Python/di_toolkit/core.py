"""Shared, READ-ONLY reporting/utilities. The checks never mutate UE assets or levels."""
import csv
import datetime
import json
import os
import re
from pathlib import Path

import unreal

VERSION = "0.1.0"
_LAST = {"report": None, "actors": [], "assets": []}


def log(msg, severity="INFO"):
    txt = "[DI Tools] {}".format(msg)
    if severity == "ERROR":
        unreal.log_error(txt)
    elif severity == "WARNING":
        unreal.log_warning(txt)
    else:
        unreal.log(txt)


def config_path():
    return Path(__file__).resolve().parents[3] / "Config" / "DI_Toolkit.json"


def load_config():
    path = config_path()
    if not path.is_file():
        raise RuntimeError("Missing DI toolkit config: {}".format(path))
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def safe_prop(obj, name, default=None):
    if obj is None:
        return default
    try:
        return obj.get_editor_property(name)
    except Exception:
        try:
            return getattr(obj, name)
        except Exception:
            return default


def path_of(obj):
    if obj is None:
        return ""
    try:
        return str(obj.get_path_name())
    except Exception:
        return str(obj)


def label_of(obj):
    if obj is None:
        return "<None>"
    for name in ("get_actor_label", "get_name"):
        try:
            return str(getattr(obj, name)())
        except Exception:
            pass
    return path_of(obj)


def actor_system():
    return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def selected_assets():
    return list(unreal.EditorUtilityLibrary.get_selected_assets())


def get_assets_in_folder(folder):
    if not folder or not folder.startswith("/Game"):
        raise ValueError("Asset folder must start with /Game")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    return [data.get_asset() for data in registry.get_assets_by_path(folder, recursive=True)]


def get_latest_report():
    return _LAST["report"]


def _outdir():
    p = Path(unreal.Paths.project_saved_dir()) / "DI_Toolkit"
    p.mkdir(parents=True, exist_ok=True)
    return p


class Report:
    def __init__(self, tool, scope):
        self.tool = tool
        self.scope = scope
        self.rows = []
        self.examined = 0
        self.actors = []
        self.assets = []

    def issue(self, severity, code, obj, detail, suggestion="", actor=None, asset=None):
        if severity not in ("INFO", "WARNING", "FAIL", "SKIP"):
            raise ValueError("Bad severity")
        row = {"severity": severity, "code": code, "object": label_of(obj),
               "object_path": path_of(obj), "detail": str(detail), "suggestion": str(suggestion)}
        self.rows.append(row)
        if severity in ("WARNING", "FAIL"):
            if actor is not None and actor not in self.actors:
                self.actors.append(actor)
            if asset is not None and asset not in self.assets:
                self.assets.append(asset)

    def save(self):
        now = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
        stem = re.sub(r"[^A-Za-z0-9_\-]", "_", self.tool)
        base = _outdir() / "{}_{}".format(now, stem)
        summary = {key: sum(1 for r in self.rows if r["severity"] == key)
                   for key in ("FAIL", "WARNING", "INFO", "SKIP")}
        payload = {"toolkit_version": VERSION, "tool": self.tool, "scope": self.scope,
                   "timestamp_local": datetime.datetime.now().isoformat(timespec="seconds"),
                   "examined": self.examined, "summary": summary, "issues": self.rows}
        with open(str(base) + ".json", "w", encoding="utf-8") as f:
            json.dump(payload, f, indent=2, ensure_ascii=False)
        with open(str(base) + ".csv", "w", encoding="utf-8-sig", newline="") as f:
            writer = csv.DictWriter(f, fieldnames=["severity", "code", "object", "object_path", "detail", "suggestion"])
            writer.writeheader()
            writer.writerows(self.rows)
        _LAST.update({"report": str(base) + ".json", "actors": list(self.actors), "assets": list(self.assets)})
        msg = "{}: objects={} FAIL={} WARNING={} INFO={} SKIP={} JSON={} CSV={}".format(
            self.tool, self.examined, summary["FAIL"], summary["WARNING"], summary["INFO"],
            summary["SKIP"], str(base) + ".json", str(base) + ".csv")
        log(msg, "WARNING" if summary["FAIL"] or summary["WARNING"] else "INFO")
        for row in self.rows[:80]:
            if row["severity"] in ("FAIL", "WARNING", "SKIP"):
                log("{} [{}] {}: {}".format(row["severity"], row["code"], row["object"], row["detail"]),
                    "WARNING" if row["severity"] in ("FAIL", "WARNING") else "INFO")
        if len(self.rows) > 80:
            log("Only first 80 findings shown. Full report is on disk.")
        return str(base) + ".json"


def select_last_problem_actors():
    actors = [a for a in _LAST["actors"] if a is not None]
    if not actors:
        log("No problem actors in latest report. Scan a level first.", "WARNING")
        return 0
    actor_system().set_selected_level_actors(actors)
    log("Selected {} problem actor(s) in Outliner.".format(len(actors)))
    return len(actors)


def focus_last_problem_assets():
    assets = [a for a in _LAST["assets"] if a is not None]
    if not assets:
        log("No problem assets in latest report.", "WARNING")
        return 0
    unreal.EditorAssetLibrary.sync_browser_to_objects([path_of(a) for a in assets])
    log("Focused {} problem asset(s) in Content Browser.".format(len(assets)))
    return len(assets)


def show_last_report():
    log("Last report: {}".format(_LAST["report"] or "No scan has run"))
    return _LAST["report"]
