#!/usr/bin/env python3
# Convierte los presets TOML de 43PR (~/.local/src/downloads-43pr/.config/43pr/themes) a colorschemes de wal
# (~/.config/wal/colorschemes/{dark,light}/43pr-<nombre>.json). Mapeo: bg/fg/cursor -> special, term0..15 -> color0..15.
import json, sys, tomllib, pathlib
src = pathlib.Path(sys.argv[1]); dst = pathlib.Path(sys.argv[2]); n = 0
for f in sorted(src.glob("*.toml")):
    t = tomllib.load(open(f, "rb")); c = t["colors"]
    out = {"special": {"background": c["bg"], "foreground": c["fg"], "cursor": c.get("cursor", c["fg"])},
           "colors": {f"color{i}": c[f"term{i}"] for i in range(16)}}
    sub = "light" if t.get("appearance") == "light" else "dark"
    p = dst / sub / f"43pr-{t.get('name', f.stem)}.json"; p.write_text(json.dumps(out)); n += 1
    print(f"{sub:5} {p.name:32} bg={c['bg']} fg={c['fg']} accent={c.get('accent')}")
print(n, "temas")
