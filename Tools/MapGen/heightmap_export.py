"""
heightmap_export.py -- turn the MapGen elevation field into a UE-Landscape-ready
16-bit heightmap.

Input : MapData/caldreth_data.npz  (elevation: float32 [0..1], 512x512)
Output: MapData/caldreth_height16.png  (16-bit greyscale PNG, UE "Import from File")
        MapData/caldreth_height.r16     (raw little-endian uint16, alt UE import)

Mapping (physical, per director's K3 RESOLVED):
  elevation 0.0 -> 16bit 0      -> Z 0 UU
  elevation 1.0 -> 16bit 65535  -> Z 90000 UU   (~900 m volcano peak)
  sea_level 0.2 -> 16bit 13107  -> Z 18000 UU   (ocean surface / Z base)
The 16-bit map is LINEAR in elevation; the Z=90000 scaling is applied by the
Landscape actor's Z transform at build time (kept out of the pixels so the same
file works at any WorldSize). Resampling to a canonical Landscape size
(505/509/1009) is a build-time decision (OPEN) -- this util emits NATIVE 512.
"""
import os
import numpy as np

_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MAPDATA = os.path.join(_ROOT, "MapData")

Z_MAX_UU = 90000.0
SEA_LEVEL = 0.2

def main():
    npz = os.path.join(MAPDATA, "caldreth_data.npz")
    elev = np.load(npz)["elevation"].astype(np.float64)
    n = elev.shape[0]

    # linear [0..1] -> [0..65535]
    clamped = np.clip(elev, 0.0, 1.0)
    h16 = np.round(clamped * 65535.0).astype(np.uint16)

    # 16-bit PNG (UE accepts I;16 greyscale)
    from PIL import Image
    Image.fromarray(h16, mode="I;16").save(os.path.join(MAPDATA, "caldreth_height16.png"))

    # raw little-endian uint16
    h16.astype("<u2").tofile(os.path.join(MAPDATA, "caldreth_height.r16"))

    # report
    print(f"grid: {n}x{n}")
    print(f"elevation f32: min={elev.min():.5f} max={elev.max():.5f}")
    print(f"16-bit:        min={int(h16.min())} max={int(h16.max())}")
    print(f"Z mapping:     0->0 UU | sea_level {SEA_LEVEL}->{SEA_LEVEL*Z_MAX_UU:.0f} UU | 1.0->{Z_MAX_UU:.0f} UU")
    frac_land = float((elev > SEA_LEVEL).mean()) * 100.0
    print(f"above sea_level ({SEA_LEVEL}): {frac_land:.1f}% of cells (dry land candidate)")
    print("wrote: caldreth_height16.png, caldreth_height.r16 -> MapData/")

if __name__ == "__main__":
    main()
