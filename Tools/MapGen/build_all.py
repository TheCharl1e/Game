"""
build_all.py -- one entry point that produces the full Caldreth deliverable:

  caldreth_map.png       visual canon (biomes + wind + rainfall panels)
  caldreth_biome.png     indexed biome mask (1 px = 1 cell) for UE import
  caldreth_data.npz      raw arrays: elevation, rainfall, biome ids
  caldreth_manifest.json config + POIs (normalized 0..1 coords) + legend

The manifest is the bridge to Unreal: a UE editor utility can read it, place
one ACaldrethZone per biome region and one marker per POI, sampling the .npz /
biome mask for exact boundaries. Coords are normalized so they map onto any
world size you pick (e.g. multiply by 1_000_00 uu for a 100 km island).
"""
import json
import os
import numpy as np

from caldreth_mapgen import MapConfig, generate, Biome
from caldreth_render import render

# Output goes to <ProjectRoot>/MapData. This script lives in <ProjectRoot>/Tools/MapGen/,
# so the project root is two directories up. Computed from __file__ -> works on Win & Linux.
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAPDATA_DIR = os.path.join(_PROJECT_ROOT, "MapData")


BIOME_NAMES = {v: k for k, v in vars(Biome).items() if isinstance(v, int)}

# How a POI kind maps onto an in-sim tag / gameplay role (documents intent).
POI_ROLE = {
    "caldera":  "FireGate_TrueName_L5",      # source of the island's true name
    "tree":     "PowerSite_Stability_L3",    # psyche buff + wider campfire sync
    "obsidian": "PowerSite_Weapons_L4",      # guarded rare weapon material
    "grey":     "Anomaly_GreyDrain",         # the strange spring (see note below)
    "spring":   "Resource_FreshWater_L1",    # river source / hydration chokepoint
}


def export(island, out_dir=None):
    if out_dir is None:
        out_dir = MAPDATA_DIR
    os.makedirs(out_dir, exist_ok=True)
    cfg = island.cfg
    n = cfg.size

    # raw arrays (float32 to keep the file small: 512x512x2 floats + int16 ~ 1.5 MB)
    np.savez_compressed(
        f"{out_dir}/caldreth_data.npz",
        elevation=island.elev.astype(np.float32),
        rainfall=island.rain.astype(np.float32),
        biome=island.biome.astype(np.int16),
    )

    # indexed biome mask as a lossless PNG (one channel == biome id)
    try:
        from PIL import Image
        Image.fromarray(island.biome.astype(np.uint8), mode="L").save(
            f"{out_dir}/caldreth_biome.png")
    except Exception:
        pass  # PIL optional; npz already carries the ids

    # POIs in normalized coords (x east-positive, y south-positive to match grid)
    pois = [{
        "name": p.name,
        "kind": p.kind,
        "role": POI_ROLE.get(p.kind, "Unset"),
        "x": round(p.yx[1] / n, 5),
        "y": round(p.yx[0] / n, 5),
    } for p in island.pois]

    manifest = {
        "island": {"use_name": "Caldreth", "true_name": "Tharruan"},
        "grid_size": n,
        "sea_level": cfg.sea_level,
        "wind": {"from": "east", "vector": [-1, 0],
                 "note": "drives orographic rainfall: wet windward (E), dry leeward (W)"},
        "biome_legend": {str(k): BIOME_NAMES[k] for k in sorted(BIOME_NAMES)
                         if k <= Biome.OASIS},
        "points_of_interest": pois,
        "grey_spring_note": (
            "Anomaly. NPCs drinking here accrue a GreyDrain stat that erodes "
            "OCEAN-five individuality (preferences flatten, true-name fades). "
            "Hook for a hive/loss-of-self mechanic in the leeward savanna."),
    }
    with open(f"{out_dir}/caldreth_manifest.json", "w") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    return manifest


if __name__ == "__main__":
    cfg = MapConfig()
    island = generate(cfg)
    os.makedirs(MAPDATA_DIR, exist_ok=True)
    render(island, os.path.join(MAPDATA_DIR, "caldreth_map.png"))
    m = export(island)
    print("Built Caldreth deliverable.")
    print(json.dumps(m["points_of_interest"], indent=2, ensure_ascii=False))
