"""Render Caldreth to a PNG: left = the island (biomes, rivers, lava, POIs,
wind), right = the rainfall field that the east wind produces."""
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap, BoundaryNorm, LinearSegmentedColormap
from matplotlib.patches import FancyArrow
from scipy.ndimage import binary_erosion

from caldreth_mapgen import MapConfig, generate, Biome, BIOME_COLORS


POI_STYLE = {
    "caldera":  dict(marker="^", color="#ff5a2a", size=240, edge="#1a0a06"),
    "tree":     dict(marker="*", color="#9be86a", size=420, edge="#10240f"),
    "obsidian": dict(marker="D", color="#1b1b1b", size=150, edge="#d8d8d8"),
    "grey":     dict(marker="o", color="#cfcfcf", size=260, edge="#3a3a3a"),
    "spring":   dict(marker="o", color="#6fd0ff", size=130, edge="#0c2a3a"),
}


def biome_image(biome: np.ndarray) -> np.ndarray:
    ids = sorted(BIOME_COLORS.keys())
    lut = {b: i for i, b in enumerate(ids)}
    cmap = ListedColormap([BIOME_COLORS[b] for b in ids])
    indexed = np.vectorize(lut.get)(biome)
    return cmap(indexed / (len(ids) - 1))


def hillshade(elev, azimuth_deg=90, altitude_deg=45):
    """Cheap hillshade; light from the EAST (azimuth 90) to echo the wind."""
    gy, gx = np.gradient(elev)
    slope = np.pi / 2 - np.arctan(np.hypot(gx, gy) * 6)
    aspect = np.arctan2(-gx, gy)
    az = np.radians(360 - azimuth_deg + 90)
    alt = np.radians(altitude_deg)
    shade = (np.sin(alt) * np.sin(slope) +
             np.cos(alt) * np.cos(slope) * np.cos(az - aspect))
    return np.clip(shade, 0, 1)


def render(island, path_png):
    cfg = island.cfg
    elev, rain, biome = island.elev, island.rain, island.biome
    land = elev >= cfg.sea_level

    fig, (axM, axR) = plt.subplots(
        1, 2, figsize=(20, 10.5), facecolor="#0e1a24",
        gridspec_kw=dict(width_ratios=[1.32, 1.0], wspace=0.04))

    # ---- LEFT: the island ------------------------------------------------- #
    img = biome_image(biome)
    shade = hillshade(elev)[..., None]
    img[..., :3] = img[..., :3] * (0.55 + 0.55 * shade)   # drape relief
    img = np.clip(img, 0, 1)
    # darken ocean flat (no hillshade noise on water)
    ocean = biome == Biome.OCEAN
    img[ocean, :3] = np.array([0.114, 0.231, 0.325])
    axM.imshow(img, origin="upper")

    # coastline outline
    coast = land ^ binary_erosion(land)
    axM.contour(land.astype(float), levels=[0.5], colors="#0a141c", linewidths=1.3)

    # POIs
    for p in island.pois:
        st = POI_STYLE.get(p.kind)
        if not st:
            continue
        y, x = p.yx
        axM.scatter(x, y, marker=st["marker"], s=st["size"], c=st["color"],
                    edgecolors=st["edge"], linewidths=1.6, zorder=5)
        axM.annotate(p.name, (x, y), xytext=(8, 8), textcoords="offset points",
                     color="#f2ead8", fontsize=11, fontweight="bold", zorder=6,
                     path_effects=[])

    # wind arrows (from the east -> pointing west)
    n = cfg.size
    for yy in np.linspace(0.12, 0.88, 5):
        axM.annotate("", xy=(0.02 * n, yy * n), xytext=(0.18 * n, yy * n),
                     arrowprops=dict(arrowstyle="-|>", color="#bfe0ff",
                                     lw=2.2, alpha=0.8))
    axM.text(0.135 * n, 0.045 * n, "WIND  E \u2192 W", color="#bfe0ff",
             fontsize=13, fontweight="bold", ha="center")

    axM.set_title("CALDRETH  \u00b7  true-name: THARRUAN",
                  color="#f2ead8", fontsize=20, fontweight="bold", pad=14)
    axM.axis("off")

    # ---- RIGHT: rainfall the wind produces -------------------------------- #
    rain_cmap = LinearSegmentedColormap.from_list(
        "rain", ["#caa46a", "#d9c98e", "#9fc06a", "#4f9e57", "#2f6e8e", "#1f3f6e"])
    rfield = np.where(land, rain, np.nan)
    axR.imshow(np.where(ocean, 0.0, np.nan), cmap="Blues", vmin=0, vmax=1)
    im = axR.imshow(rfield, cmap=rain_cmap, vmin=0, vmax=1, origin="upper")
    axR.contour(land.astype(float), levels=[0.5], colors="#0a141c", linewidths=1.2)

    # river + lava overlaid so you see them sit in the wet/dry pattern
    for (cy, cx) in island.water_path:
        axR.plot(cx, cy, ",", color="#1f5fa0")
    for (cy, cx) in island.lava_path:
        axR.plot(cx, cy, ",", color="#d8531f")

    for yy in np.linspace(0.12, 0.88, 5):
        axR.annotate("", xy=(0.02 * n, yy * n), xytext=(0.16 * n, yy * n),
                     arrowprops=dict(arrowstyle="-|>", color="#1a2a3a",
                                     lw=2.0, alpha=0.7))
    axR.set_title("RAINFALL  \u00b7  east wind drops its water on the windward slopes",
                  color="#f2ead8", fontsize=15, pad=14)
    axR.axis("off")
    cb = fig.colorbar(im, ax=axR, fraction=0.04, pad=0.02)
    cb.set_label("wet  \u2192", color="#f2ead8")
    cb.ax.yaxis.set_tick_params(color="#f2ead8")
    plt.setp(plt.getp(cb.ax.axes, "yticklabels"), color="#f2ead8")

    fig.savefig(path_png, dpi=130, bbox_inches="tight", facecolor="#0e1a24")
    plt.close(fig)
    return path_png


if __name__ == "__main__":
    import os
    _mapdata = os.path.join(
        os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "MapData")
    os.makedirs(_mapdata, exist_ok=True)
    cfg = MapConfig()
    island = generate(cfg)
    out = render(island, os.path.join(_mapdata, "caldreth_map.png"))
    print("Rendered:", out)
