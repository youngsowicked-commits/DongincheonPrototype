"""Read-only scene/mesh validation for Dongincheon building production."""
import re
import math
import unreal
from . import core


def _name_match(actor, cfg):
    name = core.label_of(actor)
    if any(x.lower() in name.lower() for x in cfg["ignore_actor_name_keywords"]):
        return False
    tags = core.safe_prop(actor, "tags", [])
    if any(str(t) == cfg["building_actor_tag"] for t in tags):
        return True
    return bool(re.search(cfg["building_name_pattern"], name, re.IGNORECASE))


def _components(actor):
    try:
        return list(actor.get_components_by_class(unreal.StaticMeshComponent))
    except Exception:
        return []


def _vector_values(v):
    return float(v.x), float(v.y), float(v.z)


def _bounds(actor):
    try:
        return actor.get_actor_bounds(False)  # origin, box_extent in world coordinates
    except Exception:
        return None


def _ground_actor(candidate, cfg):
    if candidate is None:
        return False
    tags = core.safe_prop(candidate, "tags", [])
    if any(str(t) == cfg["ground_actor_tag"] for t in tags):
        return True
    label = core.label_of(candidate).lower()
    if any(token in label for token in cfg["ground_name_keywords"]):
        return True
    try:
        return isinstance(candidate, unreal.LandscapeProxy)
    except Exception:
        return False


def _hit_actor(hit):
    try:
        return hit.get_actor()
    except Exception:
        return core.safe_prop(hit, "hit_actor")


def _ground_samples(actor, cfg, report, bottom_z, origin, extent):
    """Sample collision under AABB interior points. Explicitly NOT a placement fix.

    Ground contact checks are only reliable if road/landscape actors have Visibility
    collision and their labels/tags match the project config. Wrong hits are SKIP.
    """
    if not hasattr(unreal, "SystemLibrary"):
        report.issue("SKIP", "TRACE_NOT_AVAILABLE", actor,
                     "UE SystemLibrary tracing API not available", actor=actor)
        return
    world = unreal.EditorLevelLibrary.get_editor_world() if hasattr(unreal, "EditorLevelLibrary") else None
    if world is None:
        report.issue("SKIP", "EDITOR_WORLD_UNAVAILABLE", actor, "Could not resolve editor world")
        return
    f = max(0.0, min(0.95, float(cfg["ground_footprint_fraction"])))
    offsets = [(0, 0), (-f, -f), (f, -f), (f, f), (-f, f)]
    gaps = []
    ignored = 0
    for dx, dy in offsets:
        x, y = float(origin.x + dx * extent.x), float(origin.y + dy * extent.y)
        start = unreal.Vector(x, y, float(bottom_z + cfg["ground_trace_up_cm"]))
        end = unreal.Vector(x, y, float(bottom_z - cfg["ground_trace_down_cm"]))
        try:
            result = unreal.SystemLibrary.line_trace_single(
                world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                False, [actor], unreal.DrawDebugTrace.NONE, True)
            has_hit, hit = result[0], result[1]
        except Exception as exc:
            report.issue("SKIP", "TRACE_ERROR", actor, str(exc)[:240])
            return
        if not has_hit or not _ground_actor(_hit_actor(hit), cfg):
            ignored += 1
            continue
        p = core.safe_prop(hit, "impact_point")
        if p is None:
            ignored += 1
            continue
        gaps.append(float(bottom_z - p.z))
    if not gaps:
        report.issue("SKIP", "GROUND_UNVERIFIED", actor,
                     "No known ground hit at {} samples; label road actors or add tag {} / check Visibility collision".format(
                         len(offsets), cfg["ground_actor_tag"]),
                     "Ground geometry must have blocking Visibility collision. AABB corners are only approximate.")
        return
    if ignored:
        report.issue("INFO", "GROUND_PARTIAL_SAMPLES", actor,
                     "{} of {} ground samples ignored (not recognized as road/ground).".format(ignored, len(offsets)))
    if min(gaps) > float(cfg["ground_floating_warn_cm"]):
        report.issue("WARNING", "BUILDING_FLOATING_CANDIDATE", actor,
                     "AABB bottom above ground at {} sampled points, min gap {:.1f} cm; max {:.1f} cm".format(
                         len(gaps), min(gaps), max(gaps)),
                     "Inspect facade footing and door thresholds; AABB may include overhangs.", actor=actor)
    if max(gaps) < -float(cfg["ground_penetration_warn_cm"]):
        report.issue("WARNING", "BUILDING_PENETRATION_CANDIDATE", actor,
                     "AABB bottom below ground: closest sample {:.1f} cm; worst {:.1f} cm".format(
                         max(gaps), min(gaps)),
                     "Compare actual mesh contact points; AABB is not the foundation surface.", actor=actor)
    if (max(gaps) - min(gaps)) > float(cfg["ground_floating_warn_cm"] * 3):
        report.issue("INFO", "STEEP_GROUND_OR_IRREGULAR_FOOTPRINT", actor,
                     "Ground height spread {:.1f} cm across checked points; manual judgement required".format(
                         max(gaps) - min(gaps)))


def _inspect_actor(actor, cfg, report, with_ground=False):
    label = core.label_of(actor)
    comps = _components(actor)
    if not comps:
        report.issue("SKIP", "NO_STATIC_MESH_COMPONENT", actor,
                     "Actor has no static mesh components (possibly a procedural or skeletal building)")
    else:
        for comp in comps:
            mesh = core.safe_prop(comp, "static_mesh")
            if mesh is None:
                report.issue("WARNING", "MISSING_STATIC_MESH", actor,
                             "Component {} has no StaticMesh".format(core.label_of(comp)),
                             "Assign a mesh or remove the empty component.", actor=actor)
                continue
            combined = "{} {}".format(label, core.label_of(mesh)).lower()
            if any(x in combined for x in cfg["placeholder_name_keywords"]):
                report.issue("WARNING", "PLACEHOLDER_NAME_CANDIDATE", actor,
                             "Placeholder-like name in Actor or mesh: {}".format(core.label_of(mesh)),
                             "Verify if this is a legitimate final asset.", actor=actor)
            try:
                if not bool(comp.is_visible()):
                    report.issue("INFO", "INVISIBLE_COMPONENT", actor,
                                 "Component hidden: {}".format(core.label_of(comp)))
            except Exception:
                pass
    try:
        scale = actor.get_actor_scale3d()
        xyz = _vector_values(scale)
        if any(abs(v) < 0.01 for v in xyz):
            report.issue("WARNING", "ZERO_SCALE", actor, "Actor near-zero scale {}".format(xyz),
                         "Inspect actor scale.", actor=actor)
        elif any(v < 0 for v in xyz):
            report.issue("INFO", "NEGATIVE_SCALE", actor, "Negative scale {} (can affect normals/collision)".format(xyz))
    except Exception:
        pass
    bounds = _bounds(actor)
    if bounds:
        origin, extent = bounds
        if float(extent.z) < 1:
            report.issue("WARNING", "DEGENERATE_BOUNDS", actor,
                         "Actor bounds height {:.1f} cm".format(extent.z), actor=actor)
        if with_ground and float(extent.z) >= 1:
            _ground_samples(actor, cfg, report, origin.z - extent.z, origin, extent)
    elif with_ground:
        report.issue("SKIP", "BOUNDS_NOT_AVAILABLE", actor, "Cannot inspect ground without bounds")


def _run_actors(actors, mode, ground=False):
    cfg = core.load_config()
    report = core.Report("environment_ground" if ground else "environment_actors", mode)
    for actor in actors:
        report.examined += 1
        try:
            _inspect_actor(actor, cfg, report, ground)
        except Exception as exc:
            report.issue("SKIP", "ACTOR_API_ERROR", actor, "Actor inspect failed: {}".format(exc))
    if not actors:
        core.log("No target actors. Select buildings or configure building tags/name pattern.", "WARNING")
    return report.save()


def scan_buildings():
    cfg = core.load_config()
    actors = [actor for actor in core.actor_system().get_all_level_actors() if _name_match(actor, cfg)]
    return _run_actors(actors, "loaded_level_buildings", ground=False)


def scan_selected():
    actors = list(core.actor_system().get_selected_level_actors())
    return _run_actors(actors, "selected_level_actors", ground=False)


def scan_selected_ground():
    actors = list(core.actor_system().get_selected_level_actors())
    return _run_actors(actors, "selected_building_ground_probes", ground=True)


def _inspect_mesh(mesh, cfg, report):
    try:
        lods = int(mesh.get_num_lods())
    except Exception as exc:
        report.issue("SKIP", "MESH_RENDER_DATA_UNAVAILABLE", mesh, str(exc), asset=mesh)
        return
    if lods <= 0:
        report.issue("FAIL", "NO_MESH_LOD", mesh, "No static mesh LOD available", asset=mesh)
        return
    try:
        uv_count = int(mesh.get_num_tex_coords(0))
        if uv_count < 1:
            report.issue("FAIL", "NO_UV0", mesh, "No UV channel in LOD0", asset=mesh)
        elif uv_count == 1:
            report.issue("INFO", "ONE_UV_CHANNEL", mesh,
                         "UV0 exists; second channel is not mandatory for all Nanite/Lumen workflows")
    except Exception as exc:
        report.issue("SKIP", "UV_API_UNAVAILABLE", mesh, "UV count error: {}".format(exc))
    try:
        slots = list(mesh.get_editor_property("static_materials"))
        if not slots:
            report.issue("WARNING", "NO_MATERIAL_SLOTS", mesh, "Mesh has no material slots", asset=mesh)
        for index, slot in enumerate(slots):
            if core.safe_prop(slot, "material_interface") is None:
                report.issue("WARNING", "MATERIAL_SLOT_EMPTY", mesh,
                             "Slot {} has no material interface".format(index),
                             "Assign an intended material or explicitly whitelist default material use.", asset=mesh)
    except Exception as exc:
        report.issue("SKIP", "MATERIAL_INSPECTION_UNAVAILABLE", mesh, str(exc))
    try:
        if mesh.get_num_vertices(0) <= 0 or mesh.get_num_triangles(0) <= 0:
            report.issue("FAIL", "EMPTY_MESH_GEOMETRY", mesh, "Zero triangle or vertex count", asset=mesh)
    except Exception as exc:
        report.issue("SKIP", "GEOMETRY_API_UNAVAILABLE", mesh, str(exc))
    try:
        sections = int(mesh.get_num_sections(0))
        if sections <= 0:
            report.issue("WARNING", "NO_RENDER_SECTIONS", mesh, "LOD0 has no sections", asset=mesh)
    except Exception:
        pass
    if cfg["check_collision_info"]:
        body = core.safe_prop(mesh, "body_setup")
        if body is None:
            report.issue("INFO", "NO_BODY_SETUP", mesh,
                         "BodySetup absent; valid for purely visual meshes, check gameplay collision needs")
        else:
            flag = core.safe_prop(body, "collision_trace_flag")
            if flag is not None:
                report.issue("INFO", "COLLISION_COMPLEXITY", mesh,
                             "BodySetup collision trace flag: {}".format(flag))


def _run_meshes(meshes, mode):
    cfg = core.load_config()
    report = core.Report("static_mesh_quality", mode)
    unique = {}
    for mesh in meshes:
        if isinstance(mesh, unreal.StaticMesh):
            unique[core.path_of(mesh)] = mesh
    if len(unique) > cfg["max_asset_scan_count"]:
        core.log("Scan capped to {} assets. Narrow selection.".format(cfg["max_asset_scan_count"]), "WARNING")
    for mesh in list(unique.values())[:cfg["max_asset_scan_count"]]:
        report.examined += 1
        try:
            _inspect_mesh(mesh, cfg, report)
        except Exception as exc:
            report.issue("SKIP", "ASSET_API_ERROR", mesh, "Static mesh scan failed: {}".format(exc))
    if not unique:
        core.log("No StaticMesh assets found in target scope.", "WARNING")
    return report.save()


def scan_selected_mesh_assets():
    return _run_meshes(core.selected_assets(), "selected_content_browser_assets")


def scan_building_mesh_assets():
    cfg = core.load_config()
    actors = [a for a in core.actor_system().get_all_level_actors() if _name_match(a, cfg)]
    meshes = [core.safe_prop(comp, "static_mesh") for a in actors for comp in _components(a)]
    return _run_meshes(meshes, "referenced_by_loaded_level_buildings")
