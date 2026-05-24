"""Headless Blender renderer for .g3d files.

Loads a g3d via the g3d_support Blender add-on, sets up basic lighting +
camera, and renders to PNG. Useful for CI snapshots, asset reference
sheets, and side-by-side comparisons with reference art.

Usage (Blender 5.1+, requires the g3d_support_b510 add-on installed):

    blender --background --factory-startup \\
        --python g3d_render.py -- \\
        --g3d path/to/model.g3d \\
        --out path/to/output.png \\
        [--angle front|side|back|3q|all] \\
        [--width 400] [--height 800]

With --angle all, four PNGs are produced (front/side/back/3q) using the
out path as a prefix.

The add-on must already be installed in the user's Blender add-ons
directory (~/.config/blender/<ver>/scripts/addons/g3d_support_b510.py).
"""
import sys
import os
import math
import argparse
import importlib

import bpy
import mathutils


def find_plugin_module():
    """Locate the g3d_support add-on as an importable module."""
    # Try the modern add-on path first
    candidates = [
        os.path.expanduser("~/.config/blender/5.1/scripts/addons/g3d_support_b510.py"),
        # Add other plausible install paths here if needed.
    ]
    for path in candidates:
        if os.path.exists(path):
            spec = importlib.util.spec_from_file_location("g3d_support", path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            return mod
    raise RuntimeError(
        "g3d_support_b510.py add-on not found. Install it under "
        "~/.config/blender/<ver>/scripts/addons/ first."
    )


def parse_args():
    # Blender swallows args before "--", scripts see args after.
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Render a .g3d via Blender.")
    p.add_argument("--g3d", required=True, help="Path to a .g3d model")
    p.add_argument("--out", required=True, help="Output PNG path (or prefix when --angle=all)")
    p.add_argument("--angle", default="3q",
                   choices=("front", "side", "back", "3q", "all"),
                   help="Camera angle (default: 3q)")
    p.add_argument("--width", type=int, default=400)
    p.add_argument("--height", type=int, default=800)
    return p.parse_args(argv)


def setup_lights():
    for name, loc, energy, color in [
        ("Key", (5, -3, 8), 3.0, (1.0, 0.95, 0.85)),
        ("Fill", (-5, 3, 6), 1.5, (0.8, 0.85, 1.0)),
    ]:
        ld = bpy.data.lights.new(name=name, type="SUN")
        ld.energy = energy
        ld.color = color
        obj = bpy.data.objects.new(name, ld)
        obj.location = loc
        bpy.context.scene.collection.objects.link(obj)


def setup_camera():
    cam_data = bpy.data.cameras.new("Cam")
    cam_obj = bpy.data.objects.new("Cam", cam_data)
    bpy.context.scene.collection.objects.link(cam_obj)
    bpy.context.scene.camera = cam_obj
    return cam_obj


def render_at_angle(cam_obj, angle_deg, radius, target, out_path):
    a = math.radians(angle_deg)
    cam_obj.location = (radius * math.sin(a), -radius * math.cos(a), target.z)
    direction = target - mathutils.Vector(cam_obj.location)
    cam_obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.render.filepath = out_path
    bpy.ops.render.render(write_still=True)
    print(f"rendered {angle_deg}° -> {out_path}")


def main():
    args = parse_args()

    plugin = find_plugin_module()
    plugin.register()

    # Clear default scene
    for obj in list(bpy.context.scene.objects):
        bpy.data.objects.remove(obj, do_unlink=True)

    bpy.ops.importg3d.g3d(filepath=args.g3d)

    # Compute model bounds for camera framing
    all_pos = []
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            for v in o.data.vertices:
                all_pos.append(o.matrix_world @ v.co)
    if not all_pos:
        raise RuntimeError("no mesh data imported")
    top_z = max(p.z for p in all_pos)
    cx = sum(p.x for p in all_pos) / len(all_pos)
    cy = sum(p.y for p in all_pos) / len(all_pos)
    target = mathutils.Vector((cx, cy, top_z * 0.55))

    setup_lights()
    cam_obj = setup_camera()

    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x = args.width
    scene.render.resolution_y = args.height
    scene.render.image_settings.file_format = "PNG"

    angle_map = {"front": 180, "side": 90, "back": 0, "3q": 135}
    radius = max(top_z * 2.5, 4.0)

    if args.angle == "all":
        base, ext = os.path.splitext(args.out)
        for name, deg in angle_map.items():
            render_at_angle(cam_obj, deg, radius, target, f"{base}_{name}{ext or '.png'}")
    else:
        render_at_angle(cam_obj, angle_map[args.angle], radius, target, args.out)


if __name__ == "__main__":
    main()
