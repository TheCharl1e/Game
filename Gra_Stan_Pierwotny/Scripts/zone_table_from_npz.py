# zone_table_from_npz.py
# Replikuje flood-fill z CaldrethImportLibrary.cpp (4-connected, MinRegionPixels=8,
# bSkipOcean=false, skip biome>11) na MapData/caldreth_data.npz i raportuje per strefa:
# zone id (kolejnosc spawnu = kolejnosc napotkania), biome, area (komorki),
# mean/min/max elevation (znormalizowana 0..1 z npz, == elev/elev.max() z mapgen).
# Uzycie: python Gra_Stan_Pierwotny/Scripts/zone_table_from_npz.py
# Data: 2026-06-19
import numpy as np
from collections import deque

BIOME = {0:"OCEAN",1:"BEACH",2:"SAVANNA",3:"DESERT",4:"GRASSLAND",5:"SLOPE_FOREST",
         6:"MOUNTAIN",7:"ASH_SLOPE",8:"CALDERA",9:"RIVER",10:"LAVA",11:"OASIS"}

MIN_REGION_PIXELS = 8
USE_8 = False
SKIP_OCEAN = False
MAX_BIOME = 11

d = np.load('MapData/caldreth_data.npz')
biome = d['biome'].astype(np.int32)
elev  = d['elevation'].astype(np.float64)
H, W = biome.shape

visited = np.zeros((H, W), dtype=bool)
if USE_8:
    nb = [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)]
else:
    nb = [(1,0),(-1,0),(0,1),(0,-1)]

zones = []
skipped_noise = 0
# C++ iterates Start = 0..W*H in row-major; flatten C-order matches.
for start in range(H*W):
    r0, c0 = divmod(start, W)
    if visited[r0, c0]:
        continue
    bval = biome[r0, c0]
    # BFS collect region
    stack = deque([(r0,c0)])
    visited[r0,c0] = True
    cells = []
    while stack:
        r,c = stack.pop()
        cells.append((r,c))
        for dr,dc in nb:
            nr,nc = r+dr, c+dc
            if 0<=nr<H and 0<=nc<W and not visited[nr,nc] and biome[nr,nc]==bval:
                visited[nr,nc]=True
                stack.append((nr,nc))
    size = len(cells)
    if size < MIN_REGION_PIXELS:
        skipped_noise += 1
        continue
    if SKIP_OCEAN and bval == 0:
        continue
    if bval > MAX_BIOME:
        continue
    rs = np.array([p[0] for p in cells]); cs = np.array([p[1] for p in cells])
    ev = elev[rs, cs]
    cx = (cs.mean()+0.5)/W
    cy = (rs.mean()+0.5)/H
    zones.append(dict(biome=int(bval), name=BIOME.get(int(bval),str(bval)),
                      area=size, emean=float(ev.mean()), emin=float(ev.min()),
                      emax=float(ev.max()), cx=float(cx), cy=float(cy)))

print(f"Placed zones: {len(zones)}  (skipped noise <{MIN_REGION_PIXELS}px: {skipped_noise})")
print(f"{'id':>2} {'biome':<13}{'area':>8}{'e_mean':>9}{'e_min':>8}{'e_max':>8}{'cx':>7}{'cy':>7}")
for i,z in enumerate(zones):
    print(f"{i:>2} {z['name']:<13}{z['area']:>8}{z['emean']:>9.3f}{z['emin']:>8.3f}{z['emax']:>8.3f}{z['cx']:>7.3f}{z['cy']:>7.3f}")

print("\nPer-biome zone counts:")
from collections import Counter
cnt = Counter(z['name'] for z in zones)
for k,v in sorted(cnt.items()):
    print(f"  {k:<13} {v}")
