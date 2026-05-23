#!/usr/bin/env python3
"""Command-line utility for inspecting and editing G3D v4 files.

Subcommands:
  info <file.g3d>                       Print mesh metadata.
  verify <file.g3d>                     Round-trip read/write and report byte equality.
  dump <file.g3d> [--frame N]           Dump full data as JSON to stdout.
  uvmap <file.g3d> <texture> <out.png>  Overlay UV wireframe on the texture image.
"""

from __future__ import annotations

import argparse
import json
import os
import sys

# Allow running this file directly without packaging.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from g3dlib import G3DModel, TEX_DIFFUSE  # noqa: E402


def cmd_info(args: argparse.Namespace) -> int:
    model = G3DModel.read(args.file)
    print(f"G3D v{model.version}  type={model.model_type}  meshes={len(model.meshes)}")
    for i, m in enumerate(model.meshes):
        print(f"  mesh[{i}] name={m.name!r}")
        print(f"    frames={m.frame_count}  vertices={m.vertex_count}  indices={m.index_count}  triangles={m.index_count // 3}")
        print(f"    diffuse={m.diffuse_color}  specular={m.specular_color}  specPow={m.specular_power}  opacity={m.opacity}")
        flag_names = []
        if m.custom_color: flag_names.append("customColor")
        if m.two_sided: flag_names.append("twoSided")
        if m.properties & 4: flag_names.append("noSelect")
        if m.properties & 8: flag_names.append("glow")
        print(f"    properties={m.properties} ({','.join(flag_names) or 'none'})")
        tex_desc = []
        for slot, name in m.texture_names:
            kind = {1: "diffuse", 2: "specular", 4: "normal"}.get(slot, f"slot{slot}")
            tex_desc.append(f"{kind}={name!r}")
        print(f"    textures={m.textures} [{', '.join(tex_desc) or 'none'}]")
    return 0


def cmd_verify(args: argparse.Namespace) -> int:
    model = G3DModel.read(args.file)
    tmp = args.file + ".roundtrip.tmp"
    model.write(tmp)
    with open(args.file, "rb") as a, open(tmp, "rb") as b:
        orig = a.read()
        rt = b.read()
    os.remove(tmp)
    if orig == rt:
        print(f"OK: {args.file} round-trips byte-identical ({len(orig)} bytes)")
        return 0
    print(f"FAIL: round-trip differs (orig={len(orig)} bytes, rt={len(rt)} bytes)")
    # Find first differing byte
    for i, (x, y) in enumerate(zip(orig, rt)):
        if x != y:
            print(f"  first diff at offset {i}: 0x{x:02x} vs 0x{y:02x}")
            break
    return 1


def cmd_dump(args: argparse.Namespace) -> int:
    model = G3DModel.read(args.file)
    out = {
        "version": model.version,
        "model_type": model.model_type,
        "meshes": [],
    }
    for m in model.meshes:
        mesh_dict = {
            "name": m.name,
            "frame_count": m.frame_count,
            "vertex_count": m.vertex_count,
            "index_count": m.index_count,
            "diffuse_color": list(m.diffuse_color),
            "specular_color": list(m.specular_color),
            "specular_power": m.specular_power,
            "opacity": m.opacity,
            "properties": m.properties,
            "textures": m.textures,
            "texture_names": [{"slot": s, "name": n} for s, n in m.texture_names],
        }
        if args.geometry:
            mesh_dict["tex_coords"] = m.tex_coords
            mesh_dict["indices"] = m.indices
            if args.frame is None:
                mesh_dict["vertices"] = m.vertices
                mesh_dict["normals"] = m.normals
            else:
                vpf = m.vertex_count * 3
                start = args.frame * vpf
                end = start + vpf
                mesh_dict["frame"] = args.frame
                mesh_dict["vertices"] = m.vertices[start:end]
                mesh_dict["normals"] = m.normals[start:end]
        out["meshes"].append(mesh_dict)
    json.dump(out, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 0


def cmd_islands(args: argparse.Namespace) -> int:
    """Detect UV islands (connected components in UV space) and report them."""
    from collections import defaultdict

    model = G3DModel.read(args.file)
    report = {"file": args.file, "meshes": []}

    for mi, m in enumerate(model.meshes):
        if not (m.textures & TEX_DIFFUSE) or not m.tex_coords:
            continue

        uv_groups: dict[tuple[float, float], list[int]] = defaultdict(list)
        for vi in range(m.vertex_count):
            s = m.tex_coords[2 * vi]
            t = m.tex_coords[2 * vi + 1]
            key = (round(s, 5), round(t, 5))
            uv_groups[key].append(vi)

        uv_key_list = list(uv_groups.keys())
        key_to_group = {k: gi for gi, k in enumerate(uv_key_list)}
        vertex_group: dict[int, int] = {}
        for gi, (key, members) in enumerate(uv_groups.items()):
            for vi in members:
                vertex_group[vi] = gi

        n_groups = len(uv_key_list)
        parent = list(range(n_groups))

        def find(x: int) -> int:
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x

        def union(a: int, b: int) -> None:
            ra, rb = find(a), find(b)
            if ra != rb:
                parent[ra] = rb

        for ti in range(m.index_count // 3):
            a = m.indices[3 * ti]
            b = m.indices[3 * ti + 1]
            c = m.indices[3 * ti + 2]
            ga, gb, gc = vertex_group[a], vertex_group[b], vertex_group[c]
            union(ga, gb)
            union(gb, gc)

        root_of = [find(g) for g in range(n_groups)]

        islands_groups: dict[int, list[int]] = defaultdict(list)
        for gi, root in enumerate(root_of):
            islands_groups[root].append(gi)

        tri_count: dict[int, int] = defaultdict(int)
        for ti in range(m.index_count // 3):
            a = m.indices[3 * ti]
            tri_count[root_of[vertex_group[a]]] += 1

        islands = []
        for root, group_idxs in islands_groups.items():
            uvs = [uv_key_list[gi] for gi in group_idxs]
            ss = [uv[0] for uv in uvs]
            tt = [uv[1] for uv in uvs]
            bb_s_min, bb_s_max = min(ss), max(ss)
            bb_t_min, bb_t_max = min(tt), max(tt)
            islands.append({
                "bbox_uv": [bb_s_min, bb_t_min, bb_s_max, bb_t_max],
                "centroid_uv": [sum(ss) / len(ss), sum(tt) / len(tt)],
                "unique_uv_vertices": len(uvs),
                "triangle_count": tri_count[root],
                "area_uv": (bb_s_max - bb_s_min) * (bb_t_max - bb_t_min),
                "_root": root,
            })

        islands.sort(key=lambda x: x["area_uv"], reverse=True)
        for idx, isl in enumerate(islands):
            isl["id"] = idx
            del isl["_root"]

        report["meshes"].append({
            "mesh_index": mi,
            "mesh_name": m.name,
            "island_count": len(islands),
            "islands": islands,
        })

    if args.json:
        with open(args.json, "w") as f:
            json.dump(report, f, indent=2)
        print(f"wrote island report to {args.json}")

    if args.overlay:
        from PIL import Image, ImageDraw, ImageFont
        tex = Image.open(args.texture).convert("RGBA")
        w, h = tex.size
        overlay = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        draw = ImageDraw.Draw(overlay)

        palette = [
            (255, 80, 80), (80, 220, 80), (80, 160, 255), (255, 220, 80),
            (255, 80, 255), (80, 255, 220), (255, 160, 80), (180, 80, 255),
            (160, 255, 80), (80, 100, 255), (255, 80, 160), (220, 220, 220),
        ]
        try:
            font = ImageFont.truetype("/usr/share/fonts/TTF/DejaVuSans-Bold.ttf", 18)
        except Exception:
            font = ImageFont.load_default()

        for ms in report["meshes"]:
            for isl in ms["islands"]:
                s0, t0, s1, t1 = isl["bbox_uv"]
                x0 = s0 * (w - 1); x1 = s1 * (w - 1)
                y1 = (1.0 - t0) * (h - 1); y0 = (1.0 - t1) * (h - 1)
                color = palette[isl["id"] % len(palette)] + (255,)
                draw.rectangle([x0, y0, x1, y1], outline=color, width=2)
                cx = (x0 + x1) / 2 - 8
                cy = (y0 + y1) / 2 - 10
                label = str(isl["id"])
                draw.rectangle([cx - 2, cy - 2, cx + 22, cy + 22], fill=(0, 0, 0, 180))
                draw.text((cx, cy), label, fill=color, font=font)

        combined = Image.alpha_composite(tex, overlay)
        combined.save(args.overlay)
        print(f"wrote island overlay to {args.overlay}")

    if not args.json and not args.overlay:
        total = sum(ms["island_count"] for ms in report["meshes"])
        print(f"{total} islands across {len(report['meshes'])} mesh(es)")
        for ms in report["meshes"]:
            print(f"  mesh[{ms['mesh_index']}] {ms['mesh_name']!r}: {ms['island_count']} islands")
            for isl in ms["islands"][:10]:
                s0, t0, s1, t1 = isl["bbox_uv"]
                print(f"    #{isl['id']:>3}  tris={isl['triangle_count']:>4}  uvs={isl['unique_uv_vertices']:>4}  bbox=({s0:.3f},{t0:.3f})-({s1:.3f},{t1:.3f})  area={isl['area_uv']:.4f}")
            if len(ms["islands"]) > 10:
                print(f"    ... and {len(ms['islands']) - 10} more")
    return 0


def cmd_uvmap(args: argparse.Namespace) -> int:
    from PIL import Image, ImageDraw

    model = G3DModel.read(args.file)
    tex = Image.open(args.texture).convert("RGBA")
    w, h = tex.size

    overlay = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    colors = [
        (255, 80, 80, 220),
        (80, 220, 80, 220),
        (80, 160, 255, 220),
        (255, 220, 80, 220),
        (255, 80, 255, 220),
    ]

    drawn = 0
    for mi, m in enumerate(model.meshes):
        if not (m.textures & TEX_DIFFUSE) or not m.tex_coords:
            continue
        color = colors[mi % len(colors)]

        def uv(idx: int) -> tuple[float, float]:
            s = m.tex_coords[2 * idx]
            t = m.tex_coords[2 * idx + 1]
            # G3D / OpenGL convention: t=0 at bottom; PIL has y=0 at top.
            return (s * (w - 1), (1.0 - t) * (h - 1))

        for tri in range(m.index_count // 3):
            a, b, c = m.indices[3 * tri], m.indices[3 * tri + 1], m.indices[3 * tri + 2]
            pa, pb, pc = uv(a), uv(b), uv(c)
            draw.line([pa, pb, pc, pa], fill=color, width=1)
            drawn += 1

    combined = Image.alpha_composite(tex, overlay)
    combined.save(args.out)
    print(f"wrote {args.out} ({w}x{h}, {drawn} triangles overlaid)")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="G3D v4 inspection / edit tool")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_info = sub.add_parser("info", help="print mesh metadata")
    p_info.add_argument("file")
    p_info.set_defaults(func=cmd_info)

    p_verify = sub.add_parser("verify", help="round-trip and report byte equality")
    p_verify.add_argument("file")
    p_verify.set_defaults(func=cmd_verify)

    p_dump = sub.add_parser("dump", help="dump JSON to stdout")
    p_dump.add_argument("file")
    p_dump.add_argument("--geometry", action="store_true", help="include vertices/normals/indices/uvs")
    p_dump.add_argument("--frame", type=int, default=None, help="with --geometry, restrict vertex/normal data to one frame")
    p_dump.set_defaults(func=cmd_dump)

    p_uv = sub.add_parser("uvmap", help="render UV wireframe overlay on a texture")
    p_uv.add_argument("file", help="g3d file")
    p_uv.add_argument("texture", help="source texture image")
    p_uv.add_argument("out", help="output PNG")
    p_uv.set_defaults(func=cmd_uvmap)

    p_isl = sub.add_parser("islands", help="detect & report UV islands (connected components in UV space)")
    p_isl.add_argument("file", help="g3d file")
    p_isl.add_argument("--json", help="write island metadata to JSON file")
    p_isl.add_argument("--overlay", help="write labelled overlay PNG (requires --texture)")
    p_isl.add_argument("--texture", help="source texture for --overlay")
    p_isl.set_defaults(func=cmd_islands)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
