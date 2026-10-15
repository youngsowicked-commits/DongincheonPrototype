"""Read-only animation/Sequencer checks. Not a replacement for in-PIE QTE tests."""
import math
import unreal
from . import core


def _is_qte(obj, cfg):
    return any(x in core.label_of(obj).lower() for x in cfg["qte_name_keywords"])


def _is_cinematic(obj, cfg):
    return any(x in core.label_of(obj).lower() for x in cfg["cinematic_name_keywords"])


def _root_distance(anim, cfg):
    """Return (distance_cm, track) or (None, diagnostic), from sampled bone poses.

    Probe selected raw root bone name only. Bone translation may be in local space;
    treat the result as a HINT rather than actor world displacement.
    """
    if not hasattr(unreal, "AnimationLibrary"):
        return None, "AnimationLibrary plugin API unavailable"
    try:
        names = [str(x) for x in unreal.AnimationLibrary.get_animation_track_names(anim)]
        track = next((n for p in cfg["root_track_candidates"] for n in names if n.lower() == p.lower()), None)
        if track is None:
            return None, "No named root animation track (available: {})".format(", ".join(names[:10]))
        duration = float(anim.get_play_length())
        if duration <= 0:
            return None, "Zero playback duration"
        # No assumptions about RootMotion extraction: request raw bone poses.
        pose_a = unreal.AnimationLibrary.get_bone_pose_for_time(anim, track, 0.0, False)
        pose_b = unreal.AnimationLibrary.get_bone_pose_for_time(anim, track, duration, False)
        a = core.safe_prop(pose_a, "translation")
        b = core.safe_prop(pose_b, "translation")
        if a is None or b is None:
            return None, "Bone pose transform missing translation"
        distance = math.sqrt((b.x-a.x)**2 + (b.y-a.y)**2 + (b.z-a.z)**2)
        return round(float(distance), 2), track
    except Exception as exc:
        return None, "Root sampling failed: {}".format(exc)


def _inspect_animation(anim, cfg, report):
    skeleton = core.safe_prop(anim, "skeleton")
    if skeleton is None:
        try:
            skeleton = anim.get_skeleton()
        except Exception:
            pass
    if skeleton is None:
        report.issue("FAIL", "NO_SKELETON", anim, "Animation has no assigned Skeleton", asset=anim)
    try:
        length = float(anim.get_play_length())
        if length <= 0:
            report.issue("FAIL", "ZERO_ANIM_LENGTH", anim, "No animation duration", asset=anim)
    except Exception as exc:
        report.issue("SKIP", "ANIM_LENGTH_UNAVAILABLE", anim, str(exc))
    enabled = core.safe_prop(anim, "enable_root_motion", None)
    force_lock = core.safe_prop(anim, "force_root_lock", None)
    normalized = core.safe_prop(anim, "use_normalized_root_motion_scale", None)
    report.issue("INFO", "ROOT_MOTION_FLAGS", anim,
                 "enable_root_motion={}, force_root_lock={}, normalized_scale={}".format(
                     enabled, force_lock, normalized))
    is_qte = _is_qte(anim, cfg)
    if is_qte:
        distance, diagnostic = _root_distance(anim, cfg)
        if distance is None:
            report.issue("SKIP", "ROOT_MOVEMENT_UNDETERMINED", anim, diagnostic,
                         "Verify root track name and animation visualization in Unreal.")
        else:
            report.issue("INFO", "ROOT_TRACK_DISTANCE", anim,
                         "Raw root track '{}' start-to-end displacement: {:.2f} cm".format(diagnostic, distance))
            if enabled is False and distance > 5.0 and cfg["warn_qte_root_motion_disabled_when_root_moves"]:
                report.issue("WARNING", "MOVING_ROOT_MOTION_DISABLED", anim,
                             "QTE root translates {:.2f} cm but enable_root_motion=False".format(distance),
                             "Do not flip automatically: Sequencer and runtime Montage have different root-motion requirements.",
                             asset=anim)


def scan_selected_animations():
    cfg = core.load_config()
    report = core.Report("animation_quality", "selected_content_browser_anim_sequences")
    selected = [a for a in core.selected_assets() if isinstance(a, unreal.AnimSequence)]
    if not selected:
        core.log("No AnimSequence selected in Content Browser.", "WARNING")
    for anim in selected[:cfg["max_asset_scan_count"]]:
        report.examined += 1
        try:
            _inspect_animation(anim, cfg, report)
        except Exception as exc:
            report.issue("SKIP", "ANIMATION_API_ERROR", anim, str(exc))
    return report.save()


def _tracks_from_sequence(seq):
    for fn in ("get_master_tracks", "get_tracks"):
        try:
            return list(getattr(seq, fn)())
        except Exception:
            pass
    try:
        return list(seq.get_movie_scene().get_master_tracks())
    except Exception:
        return None


def _bindings_from_sequence(seq):
    try:
        return list(seq.get_bindings())
    except Exception:
        return None


def _name_binding(binding):
    for fn in ("get_display_name", "get_name"):
        try:
            return str(getattr(binding, fn)())
        except Exception:
            pass
    return str(binding)


def _is_a(obj, cls):
    try:
        return cls is not None and isinstance(obj, cls)
    except Exception:
        return False


def _inspect_sequence(seq, cfg, report):
    bindings = _bindings_from_sequence(seq)
    if bindings is None:
        report.issue("SKIP", "SEQUENCE_BINDINGS_UNAVAILABLE", seq,
                     "Unable to inspect bindings. Confirm Sequencer Scripting plugin enabled.")
        return
    if not bindings:
        report.issue("WARNING", "NO_SEQUENCE_BINDINGS", seq, "Sequence has no object bindings",
                     "Check if cinematic consists only of nested SubSequences or intentionally has no actors.", asset=seq)
    binding_names = [_name_binding(b) for b in bindings]
    for keyword in cfg["cinematic_required_binding_keywords"]:
        if not any(keyword.lower() in name.lower() for name in binding_names):
            report.issue("WARNING", "EXPECTED_BINDING_NAME_MISSING", seq,
                         "Missing binding containing configured text: {}".format(keyword),
                         "Names are only a convention; confirm dynamic bindings in runtime.", asset=seq)
    top_tracks = _tracks_from_sequence(seq)
    if top_tracks is None:
        report.issue("SKIP", "MASTER_TRACKS_UNAVAILABLE", seq,
                     "Could not read root tracks; camera cut evaluation unavailable")
        top_tracks = []
    cam_cls = getattr(unreal, "MovieSceneCameraCutTrack", None)
    camera_tracks = [t for t in top_tracks if _is_a(t, cam_cls) or "CameraCut" in type(t).__name__]
    # Fallback in newer Sequencer APIs.
    if not camera_tracks:
        try:
            camera_tracks = list(seq.find_tracks_by_type(cam_cls)) if cam_cls else []
        except Exception:
            pass
    if cfg["warn_no_camera_cut"] and _is_cinematic(seq, cfg) and not camera_tracks and top_tracks is not None:
        report.issue("WARNING", "NO_CAMERA_CUT_TRACK", seq,
                     "No Camera Cut Track found in root sequence",
                     "For SubSequence-only projects, confirm the camera is controlled inside the child sequence.",
                     asset=seq)
    for track in camera_tracks:
        try:
            if not list(track.get_sections()):
                report.issue("WARNING", "EMPTY_CAMERA_CUT_TRACK", seq,
                             "Camera Cut Track has no sections", asset=seq)
        except Exception:
            pass
    anim_track_cls = getattr(unreal, "MovieSceneSkeletalAnimationTrack", None)
    tracks_checked = 0
    for binding in bindings:
        try:
            tracks = list(binding.get_tracks())
        except Exception:
            continue
        for track in tracks:
            if not (_is_a(track, anim_track_cls) or "SkeletalAnimationTrack" in type(track).__name__):
                continue
            tracks_checked += 1
            try:
                sections = list(track.get_sections())
            except Exception:
                sections = []
            if not sections:
                report.issue("WARNING", "EMPTY_SKELETAL_ANIM_TRACK", seq,
                             "Binding '{}' has no animation sections".format(_name_binding(binding)), asset=seq)
            for section in sections:
                params = core.safe_prop(section, "params")
                if params is None:
                    report.issue("SKIP", "ANIM_PARAMS_UNAVAILABLE", seq,
                                 "Could not read animation params for binding '{}'".format(_name_binding(binding)))
                elif core.safe_prop(params, "animation") is None:
                    report.issue("WARNING", "EMPTY_ANIMATION_SECTION", seq,
                                 "Animation section has no asset, binding '{}'".format(_name_binding(binding)),
                                 "Assign an animation to the section.", asset=seq)
    report.issue("INFO", "SEQUENCE_STRUCTURE", seq,
                 "{} binding(s), {} camera-cut track(s), {} skeletal animation track(s)".format(
                     len(bindings), len(camera_tracks), tracks_checked))
    # Slomo may be driven in C++ or Blueprint, so lack of a track is informational only.
    if _is_qte(seq, cfg):
        has_slowmo = any("Slomo" in type(t).__name__ or "TimeDilation" in type(t).__name__
                          for t in top_tracks)
        report.issue("INFO", "QTE_SLOWMO_OWNERSHIP", seq,
                     "Slomo track in root Sequence: {}. Gameplay Time Dilation is not statically verified; PIE test required.".format(has_slowmo),
                     "Manually test success/fail/interruption, including input and camera restoration.")
    report.issue("INFO", "BINDING_RUNTIME_LIMITATION", seq,
                 "Binding name/track checks do NOT prove Player/Boss binds correctly to live actors in PIE.",
                 "Test actual gameplay Actor references and camera transition during PIE.")


def _run_sequences(sequences, scope):
    cfg = core.load_config()
    report = core.Report("sequencer_quality", scope)
    unique = {}
    for seq in sequences:
        if isinstance(seq, unreal.LevelSequence):
            unique[core.path_of(seq)] = seq
    if not unique:
        core.log("No LevelSequence target. Select in Content Browser or open a Sequencer.", "WARNING")
    for seq in list(unique.values())[:cfg["max_asset_scan_count"]]:
        report.examined += 1
        try:
            _inspect_sequence(seq, cfg, report)
        except Exception as exc:
            report.issue("SKIP", "SEQUENCER_API_ERROR", seq, str(exc))
    return report.save()


def scan_selected_sequences():
    return _run_sequences(core.selected_assets(), "selected_content_browser_level_sequences")


def scan_current_sequence():
    seq = None
    try:
        seq = unreal.LevelSequenceEditorBlueprintLibrary.get_current_level_sequence()
    except Exception as exc:
        core.log("Could not query open Sequencer: {}".format(exc), "WARNING")
    return _run_sequences([seq] if seq else [], "open_sequencer")
