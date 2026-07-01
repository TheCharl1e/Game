"""
Caldreth procedural map generator.

Builds a volcanic island from first principles:
  - volcano at the centre pushes terrain up into rough concentric rings
  - WIND FROM THE EAST drives orographic rainfall: wet windward (east) slopes,
    a rain shadow and desert on the leeward (west) side
  - a single major river is born in the wet highlands and is routed across the
    leeward desert (Nile archetype) feeding a savanna
  - a lava river runs from the caldera to the sea as the island's hard border
  - points of interest (Great Tree, Obsidian Fields, the Grey Spring, passes)

The output is BOTH:
  - a rendered PNG (visual canon of the island)
  - raw numpy arrays (elevation, rainfall, biome ids) that a UE import step can
    later sample to drive ACaldrethZone placement.

This is a generation TOOL, not the runtime sim, so readability beats micro-opt.
"""

from __future__ import annotations
from dataclasses import dataclass, field
import numpy as np


# --------------------------------------------------------------------------- #
#  CONFIG (data-driven: tweak the island from here, no magic numbers in logic) #
# --------------------------------------------------------------------------- #
@dataclass
class MapConfig:
    size: int = 512                 # grid resolution (size x size cells)
    seed: int = 7                   # master RNG seed
    sea_level: float = 0.200        # elevation below this is ocean

    # Volcano shape. Centre is offset to break perfect symmetry.
    peak_x: float = 0.55            # 0..1, fraction of width
    peak_y: float = 0.50            # 0..1, fraction of height
    cone_radius: float = 0.74       # how far the cone reaches (in normalised units)
    peak_height: float = 1.00

    # Windward/leeward asymmetry. Wind is FROM THE EAST (+x is east here).
    # East slopes steeper & wetter, west slopes gentler & drier.
    wind_from_east: bool = True
    windward_steepness: float = 1.35
    leeward_steepness: float = 0.75

    # Orographic rainfall model.
    rain_base: float = 0.048        # baseline rain dropped per land cell
    rain_orographic: float = 1.6    # extra rain per unit of uphill gradient
    rain_depletion: float = 0.42    # how fast an air parcel loses moisture
    moisture_recharge: float = 0.10 # moisture regained per ocean cell crossed

    # Terrain rings (as elevation thresholds above sea level, 0..1 of land range)
    caldera_rim: float = 0.93       # above this near the peak = caldera/ash
    ash_band: float = 0.80
    mountain_band: float = 0.62

    # Rainfall thresholds for the vegetated bands (lower terrain)
    rain_forest: float = 0.40       # wet -> dense slope forest
    rain_grass: float = 0.22        # mid -> lowland grassland
    rain_savanna: float = 0.060     # dry -> savanna
    # below rain_savanna -> desert

    beach_band: float = 0.04        # elevation band just above sea level

    noise_octaves: int = 6
    noise_strength: float = 0.48    # how much noise distorts the clean cone


# Biome ids (kept as a flat enum so the same ints survive into the UE data layer)
class Biome:
    OCEAN = 0
    BEACH = 1
    SAVANNA = 2
    DESERT = 3
    GRASSLAND = 4
    SLOPE_FOREST = 5
    MOUNTAIN = 6
    ASH_SLOPE = 7
    CALDERA = 8
    RIVER = 9
    LAVA = 10
    OASIS = 11      # riparian green along a river inside an otherwise dry band

BIOME_COLORS = {
    Biome.OCEAN:        "#1d3b53",
    Biome.BEACH:        "#dcc790",
    Biome.SAVANNA:      "#d8c24a",   # golden grass
    Biome.DESERT:       "#c2904f",   # warm sand/ochre
    Biome.GRASSLAND:    "#86ab50",
    Biome.SLOPE_FOREST: "#2f5d3a",
    Biome.MOUNTAIN:     "#837a6d",
    Biome.ASH_SLOPE:    "#4a423f",
    Biome.CALDERA:      "#3a1410",
    Biome.RIVER:        "#3f7fb0",
    Biome.LAVA:         "#d8531f",
    Biome.OASIS:        "#4f8f3f",
}


# --------------------------------------------------------------------------- #
#  NOISE  (fractal value noise built only on numpy -> zero extra dependencies) #
# --------------------------------------------------------------------------- #
def _value_noise_layer(size: int, period: int, rng: np.random.Generator) -> np.ndarray:
    """One octave: random lattice upsampled with smooth (cosine) interpolation."""
    cells = max(2, size // period)
    lattice = rng.random((cells + 2, cells + 2))

    ys = np.linspace(0, cells, size, endpoint=False)
    xs = np.linspace(0, cells, size, endpoint=False)
    gy, gx = np.meshgrid(ys, xs, indexing="ij")

    y0, x0 = gy.astype(int), gx.astype(int)
    fy, fx = gy - y0, gx - x0
    # cosine smoothing of the fractional part
    sy = (1 - np.cos(fy * np.pi)) * 0.5
    sx = (1 - np.cos(fx * np.pi)) * 0.5

    v00 = lattice[y0,     x0]
    v01 = lattice[y0,     x0 + 1]
    v10 = lattice[y0 + 1, x0]
    v11 = lattice[y0 + 1, x0 + 1]
    top = v00 * (1 - sx) + v01 * sx
    bot = v10 * (1 - sx) + v11 * sx
    return top * (1 - sy) + bot * sy


def fractal_noise(size: int, octaves: int, rng: np.random.Generator) -> np.ndarray:
    """Sum octaves of value noise, each higher freq + lower amplitude."""
    out = np.zeros((size, size))
    amp, total = 1.0, 0.0
    period = size // 2
    for _ in range(octaves):
        out += amp * _value_noise_layer(size, max(2, period), rng)
        total += amp
        amp *= 0.5
        period = max(2, period // 2)
    out /= total
    # normalise 0..1
    out -= out.min()
    out /= max(1e-9, out.max())
    return out


# --------------------------------------------------------------------------- #
#  TERRAIN                                                                     #
# --------------------------------------------------------------------------- #
def build_elevation(cfg: MapConfig) -> np.ndarray:
    """Volcanic cone + windward/leeward skew + fractal distortion, then masked
    into an island by a radial falloff so the edges are sea."""
    n = cfg.size
    rng = np.random.default_rng(cfg.seed)

    ys = np.linspace(0, 1, n)
    xs = np.linspace(0, 1, n)
    gy, gx = np.meshgrid(ys, xs, indexing="ij")

    dx = gx - cfg.peak_x
    dy = gy - cfg.peak_y

    # Skew distance so east slopes are steeper than west slopes.
    # +dx is east. Wind from east -> compress east side (steeper).
    skew = np.where(dx > 0, cfg.windward_steepness, cfg.leeward_steepness)
    dist = np.sqrt((dx * skew) ** 2 + dy ** 2) / cfg.cone_radius

    cone = np.clip(1.0 - dist, 0.0, 1.0) ** 1.6 * cfg.peak_height

    noise = fractal_noise(n, cfg.noise_octaves, rng)
    elev = cone * (1 - cfg.noise_strength) + cone * noise * cfg.noise_strength
    # extra low-freq warp so the coastline is ragged, not a clean circle
    elev += (noise - 0.5) * 0.12

    # radial island falloff -> guarantees ocean at the frame edge
    edge = np.sqrt((gx - 0.5) ** 2 + (gy - 0.5) ** 2) / 0.95
    elev *= np.clip(1.0 - edge ** 4, 0.0, 1.0)

    elev = np.clip(elev, 0.0, None)
    elev /= elev.max()
    return elev


# --------------------------------------------------------------------------- #
#  RAINFALL  (orographic, wind from the east)                                  #
# --------------------------------------------------------------------------- #
def build_rainfall(cfg: MapConfig, elev: np.ndarray) -> np.ndarray:
    """March each row in the wind direction carrying a moisture budget.
    Rising terrain wrings rain out of the air; the far (leeward) side starves."""
    n = cfg.size
    rain = np.zeros_like(elev)
    land = elev >= cfg.sea_level

    # wind from east -> parcels travel east->west -> iterate x from high to low
    x_order = range(n - 1, -1, -1) if cfg.wind_from_east else range(n)
    x_prev_offset = +1 if cfg.wind_from_east else -1

    for y in range(n):
        moisture = 1.0
        prev_elev = cfg.sea_level
        for x in x_order:
            if not land[y, x]:
                moisture = min(1.0, moisture + cfg.moisture_recharge)
                prev_elev = elev[y, x]
                continue
            uphill = max(0.0, elev[y, x] - prev_elev)
            drop = moisture * (cfg.rain_base + cfg.rain_orographic * uphill)
            rain[y, x] = drop
            moisture = max(0.0, moisture - drop * cfg.rain_depletion)
            prev_elev = elev[y, x]

    # smooth a touch so bands aren't striped, then normalise over land.
    # Normalise by a high percentile (not the max): a few orographic spikes on
    # the windward coast would otherwise compress the whole inland gradient.
    from scipy.ndimage import gaussian_filter
    rain = gaussian_filter(rain, sigma=2.5)
    if land.any():
        ref = np.percentile(rain[land], 82)
        if ref > 0:
            rain = np.clip(rain / ref, 0.0, 1.0)
    rain[~land] = 0.0
    return rain


# --------------------------------------------------------------------------- #
#  RIVERS  (steepest descent to the sea)                                       #
# --------------------------------------------------------------------------- #
def trace_river(cfg: MapConfig, elev: np.ndarray, start_yx, max_len=4000, descent=None):
    """Follow steepest descent from a source down to sea level / a sink.
    `descent` lets us route on a meander-perturbed field while the stop test
    still uses real elevation (so the river ends at the real coast)."""
    field = elev if descent is None else descent
    n = cfg.size
    y, x = start_yx
    path = [(y, x)]
    seen = set(path)
    for _ in range(max_len):
        if elev[y, x] < cfg.sea_level:
            break
        best, best_e = None, field[y, x]
        for ny in (y - 1, y, y + 1):
            for nx in (x - 1, x, x + 1):
                if (ny, nx) == (y, x) or not (0 <= ny < n and 0 <= nx < n):
                    continue
                if field[ny, nx] < best_e and (ny, nx) not in seen:
                    best, best_e = (ny, nx), field[ny, nx]
        if best is None:           # stuck in a pit -> stop (would be a lake)
            break
        y, x = best
        path.append((y, x))
        seen.add((y, x))
    return path


def pick_leeward_source(cfg: MapConfig, elev: np.ndarray):
    """Find a high source on the LEEWARD (west) upper slope so its descent path
    runs out across the dry side -> a river through the desert."""
    n = cfg.size
    land = elev >= cfg.sea_level
    # candidate band: high-ish elevation, on the west half (leeward)
    high = (elev > 0.6) & (elev < 0.86) & land
    west = np.zeros_like(land)
    west[:, : int(n * cfg.peak_x)] = True
    cand = np.argwhere(high & west)
    if len(cand) == 0:
        cand = np.argwhere(high)
    # choose the candidate furthest west to maximise desert crossing
    cand = cand[np.argsort(cand[:, 1])]
    return tuple(cand[0])


# --------------------------------------------------------------------------- #
#  LAVA RIVER  (caldera -> coast, the hard border)                            #
# --------------------------------------------------------------------------- #
def trace_lava(cfg: MapConfig, elev: np.ndarray):
    """Steepest descent from the peak; this becomes the impassable lava border."""
    n = cfg.size
    peak = (int(cfg.peak_y * n), int(cfg.peak_x * n))
    # nudge the start a little south-west so it doesn't overlap the water river
    py = min(n - 1, peak[0] + int(0.05 * n))
    px = max(0, peak[1] - int(0.02 * n))
    return trace_river(cfg, elev, (py, px))


# --------------------------------------------------------------------------- #
#  BIOME CLASSIFICATION                                                        #
# --------------------------------------------------------------------------- #
def classify(cfg: MapConfig, elev: np.ndarray, rain: np.ndarray) -> np.ndarray:
    n = cfg.size
    biome = np.full((n, n), Biome.OCEAN, dtype=np.int16)
    land = elev >= cfg.sea_level

    # land elevation normalised 0..1 over the land range
    land_e = np.zeros_like(elev)
    lo, hi = cfg.sea_level, elev.max()
    land_e[land] = (elev[land] - lo) / max(1e-9, hi - lo)

    biome[land & (land_e < cfg.beach_band)] = Biome.BEACH

    mid = land & (land_e >= cfg.beach_band)
    # high terrain rings (rainfall-independent: volcanic rock)
    biome[mid & (land_e >= cfg.caldera_rim)] = Biome.CALDERA
    biome[mid & (land_e >= cfg.ash_band) & (land_e < cfg.caldera_rim)] = Biome.ASH_SLOPE
    biome[mid & (land_e >= cfg.mountain_band) & (land_e < cfg.ash_band)] = Biome.MOUNTAIN

    # lower/mid terrain: classified by rainfall
    veg = mid & (land_e < cfg.mountain_band)
    biome[veg & (rain >= cfg.rain_forest)] = Biome.SLOPE_FOREST
    biome[veg & (rain >= cfg.rain_grass) & (rain < cfg.rain_forest)] = Biome.GRASSLAND
    biome[veg & (rain >= cfg.rain_savanna) & (rain < cfg.rain_grass)] = Biome.SAVANNA
    biome[veg & (rain < cfg.rain_savanna)] = Biome.DESERT
    return biome


def stamp_path(biome, path, value, width=1):
    n = biome.shape[0]
    for (y, x) in path:
        for dy in range(-width, width + 1):
            for dx in range(-width, width + 1):
                yy, xx = y + dy, x + dx
                if 0 <= yy < n and 0 <= xx < n:
                    biome[yy, xx] = value


def valley_into_savanna(cfg, biome, elev, path, radius=9):
    """The river civilises the desert it crosses: a broad band of savanna
    follows the water, the lush counterpart to the dead dunes around it."""
    n = biome.shape[0]
    r2 = radius * radius
    for (y, x) in path:
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                if dy * dy + dx * dx > r2:
                    continue
                yy, xx = y + dy, x + dx
                if 0 <= yy < n and 0 <= xx < n and elev[yy, xx] >= cfg.sea_level:
                    if biome[yy, xx] == Biome.DESERT:
                        biome[yy, xx] = Biome.SAVANNA


def greenify_river_banks(cfg, biome, elev, path, radius=4):
    """Turn dry land next to the water river into an oasis ribbon."""
    n = biome.shape[0]
    for (y, x) in path:
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                yy, xx = y + dy, x + dx
                if 0 <= yy < n and 0 <= xx < n and biome[yy, xx] in (Biome.DESERT, Biome.SAVANNA):
                    if elev[yy, xx] >= cfg.sea_level:
                        biome[yy, xx] = Biome.OASIS


# --------------------------------------------------------------------------- #
#  POINTS OF INTEREST                                                          #
# --------------------------------------------------------------------------- #
@dataclass
class POI:
    name: str
    yx: tuple
    kind: str       # used for marker style + later UE tag


def place_pois(cfg, elev, rain, biome, water_path, lava_path) -> list[POI]:
    n = cfg.size
    land = elev >= cfg.sea_level
    pois: list[POI] = []

    peak = (int(cfg.peak_y * n), int(cfg.peak_x * n))
    pois.append(POI("Caldera (Tharruan)", peak, "caldera"))

    # Great Tree: wettest forest cell on the windward (east) upper slope
    forest = (biome == Biome.SLOPE_FOREST)
    east = np.zeros_like(land); east[:, int(n * cfg.peak_x):] = True
    cand = forest & east & (elev > 0.45)
    if cand.any():
        idx = np.argwhere(cand)
        # pick high + wet
        score = elev[cand] * 0.5 + rain[cand] * 0.5
        pois.append(POI("Great Tree", tuple(idx[np.argmax(score)]), "tree"))

    # Obsidian Fields: deep in the leeward desert, but pulled off the coast
    desert = (biome == Biome.DESERT)
    if desert.any():
        from scipy.ndimage import distance_transform_edt
        inland = distance_transform_edt(land)            # cells from the sea
        idx = np.argwhere(desert & (inland > 14))
        if len(idx) == 0:
            idx = np.argwhere(desert)
        # furthest west among sufficiently inland desert
        pick = idx[np.argmin(idx[:, 1])]
        pois.append(POI("Obsidian Fields", tuple(pick), "obsidian"))

    # The Grey Spring: in the savanna, near the river (a strange water source)
    savanna = (biome == Biome.SAVANNA) | (biome == Biome.OASIS)
    if savanna.any() and water_path:
        # find a river cell whose surroundings are savanna/oasis
        for (y, x) in water_path[len(water_path)//2:]:
            yy0, yy1 = max(0, y-6), min(n, y+6)
            xx0, xx1 = max(0, x-6), min(n, x+6)
            if savanna[yy0:yy1, xx0:xx1].any():
                pois.append(POI("The Grey Spring", (y, x), "grey"))
                break

    # River mouth / source markers
    if water_path:
        pois.append(POI("River Source", water_path[0], "spring"))

    return pois


# --------------------------------------------------------------------------- #
#  DRIVER                                                                      #
# --------------------------------------------------------------------------- #
@dataclass
class Island:
    cfg: MapConfig
    elev: np.ndarray
    rain: np.ndarray
    biome: np.ndarray
    water_path: list
    lava_path: list
    pois: list = field(default_factory=list)


def generate(cfg: MapConfig) -> Island:
    elev = build_elevation(cfg)
    rain = build_rainfall(cfg, elev)
    biome = classify(cfg, elev, rain)

    # meander field: a low-frequency perturbation so the river wanders instead
    # of running dead straight down the cone
    rng = np.random.default_rng(cfg.seed + 99)
    meander = fractal_noise(cfg.size, 3, rng)
    descent = elev - 0.11 * (meander - 0.5)

    src = pick_leeward_source(cfg, elev)
    water_path = trace_river(cfg, elev, src, descent=descent)
    lava_path = trace_lava(cfg, elev)

    # the river brings life across the dry side: carve a SAVANNA valley around
    # it where it crosses desert, then a tighter oasis/green core, then water
    valley_into_savanna(cfg, biome, elev, water_path, radius=15)
    greenify_river_banks(cfg, biome, elev, water_path, radius=5)
    stamp_path(biome, water_path, Biome.RIVER, width=1)
    stamp_path(biome, lava_path, Biome.LAVA, width=1)

    pois = place_pois(cfg, elev, rain, biome, water_path, lava_path)
    return Island(cfg, elev, rain, biome, water_path, lava_path, pois)


if __name__ == "__main__":
    cfg = MapConfig()
    island = generate(cfg)
    print("Generated Caldreth.")
    for p in island.pois:
        print(f"  POI {p.kind:9s} {p.name:22s} at {p.yx}")
    # quick biome histogram sanity check
    ids, counts = np.unique(island.biome, return_counts=True)
    name = {v: k for k, v in vars(Biome).items() if isinstance(v, int)}
    print("Biome coverage:")
    for i, c in zip(ids, counts):
        print(f"  {name.get(i, i):12s} {100*c/island.biome.size:5.1f}%")
