#!/usr/bin/env python3
import csv
import math
import sys
from pathlib import Path


def fmt_num(value: float) -> str:
    if value >= 1000:
        return f"{int(value):,}"
    if value >= 10:
        return f"{value:.1f}"
    if value >= 1:
        return f"{value:.2f}"
    return f"{value:.3f}"


def read_rows(csv_path: Path, codim: int):
    rows = []
    with csv_path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            if int(row["codim"]) != codim:
                continue
            row["max_degree"] = int(row["max_degree"])
            row["n"] = int(row["n"])
            for key in (
                "cpp_total_sec",
                "m2_total_sec",
                "cpp_mem_kb",
                "m2_mem_kb",
            ):
                row[key] = float(row[key])
            rows.append(row)
    rows.sort(key=lambda row: row["n"])
    return rows


def x_scale(n: float, min_n: float, max_n: float, left: float, width: float) -> float:
    if max_n == min_n:
        return left + width / 2
    return left + (n - min_n) * width / (max_n - min_n)


def y_scale_linear(value: float, min_v: float, max_v: float, top: float, height: float) -> float:
    if max_v == min_v:
        return top + height / 2
    return top + height - (value - min_v) * height / (max_v - min_v)


def y_scale_log(value: float, min_v: float, max_v: float, top: float, height: float) -> float:
    log_min = math.log10(min_v)
    log_max = math.log10(max_v)
    if log_max == log_min:
        return top + height / 2
    return top + height - (math.log10(value) - log_min) * height / (log_max - log_min)


def polyline(points, color: str) -> str:
    pts = " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
    return (
        f'<polyline fill="none" stroke="{color}" stroke-width="3" '
        f'stroke-linecap="round" stroke-linejoin="round" points="{pts}" />'
    )


def circle(x: float, y: float, color: str) -> str:
    return f'<circle cx="{x:.2f}" cy="{y:.2f}" r="4.5" fill="{color}" />'


def text(x: float, y: float, label: str, size: int = 14, anchor: str = "start", weight: str = "normal") -> str:
    safe = (
        label.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
    )
    return (
        f'<text x="{x:.2f}" y="{y:.2f}" font-family="Helvetica, Arial, sans-serif" '
        f'font-size="{size}" text-anchor="{anchor}" font-weight="{weight}" fill="#111827">{safe}</text>'
    )


def line(x1: float, y1: float, x2: float, y2: float, color: str = "#d1d5db", width: float = 1.0, dash: str = "") -> str:
    dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
    return (
        f'<line x1="{x1:.2f}" y1="{y1:.2f}" x2="{x2:.2f}" y2="{y2:.2f}" '
        f'stroke="{color}" stroke-width="{width}"{dash_attr} />'
    )


def rect(x: float, y: float, w: float, h: float, fill: str, stroke: str = "none") -> str:
    return f'<rect x="{x:.2f}" y="{y:.2f}" width="{w:.2f}" height="{h:.2f}" fill="{fill}" stroke="{stroke}" />'


def generate_svg(rows, codim: int, out_path: Path):
    width = 1100
    height = 860
    margin_left = 90
    margin_right = 40
    panel_width = width - margin_left - margin_right
    runtime_top = 120
    runtime_height = 250
    memory_top = 470
    memory_height = 250
    plot_left = margin_left
    plot_right = margin_left + panel_width

    cpp_color = "#0f766e"
    m2_color = "#b91c1c"
    grid_color = "#e5e7eb"

    min_n = min(row["n"] for row in rows)
    max_n = max(row["n"] for row in rows)

    runtime_vals = [row["cpp_total_sec"] for row in rows] + [row["m2_total_sec"] for row in rows]
    runtime_min = min(runtime_vals) * 0.8
    runtime_max = max(runtime_vals) * 1.2
    runtime_ticks = [0.005, 0.01, 0.05, 0.1, 0.5, 1.0]
    runtime_ticks = [tick for tick in runtime_ticks if runtime_min <= tick <= runtime_max]
    if runtime_max > 1.0:
        runtime_ticks.append(2.0)
    runtime_ticks = sorted(set(runtime_ticks))

    mem_vals = [row["cpp_mem_kb"] for row in rows] + [row["m2_mem_kb"] for row in rows]
    mem_min = 0.0
    mem_max = max(mem_vals) * 1.08
    mem_ticks = [0, 20000, 40000, 60000, 80000, 100000]

    parts = []
    parts.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">')
    parts.append(rect(0, 0, width, height, "#f8fafc"))
    parts.append(text(40, 45, f"Codimension {codim}: runtime and memory vs number of tested sequences (n)", 24, weight="bold"))
    parts.append(text(40, 72, "Runtime panel uses a log scale so C++ and Macaulay2 are both visible.", 14))
    parts.append(text(40, 92, "Source: data/processed/benchmarks/builtin_m2_matrix.csv", 14))

    legend_y = 40
    parts.append(line(760, legend_y, 790, legend_y, cpp_color, 3))
    parts.append(circle(775, legend_y, cpp_color))
    parts.append(text(800, legend_y + 5, "C++"))
    parts.append(line(870, legend_y, 900, legend_y, m2_color, 3))
    parts.append(circle(885, legend_y, m2_color))
    parts.append(text(910, legend_y + 5, "M2 built-in"))

    parts.append(text(40, 110, "Runtime (seconds, log scale)", 18, weight="bold"))
    parts.append(rect(plot_left, runtime_top, panel_width, runtime_height, "#ffffff", "#d1d5db"))
    for tick in runtime_ticks:
        y = y_scale_log(tick, runtime_min, runtime_max, runtime_top, runtime_height)
        parts.append(line(plot_left, y, plot_right, y, grid_color, 1))
        parts.append(text(plot_left - 12, y + 5, fmt_num(tick), 12, anchor="end"))

    cpp_runtime_pts = []
    m2_runtime_pts = []
    for row in rows:
        x = x_scale(row["n"], min_n, max_n, plot_left, panel_width)
        cpp_y = y_scale_log(row["cpp_total_sec"], runtime_min, runtime_max, runtime_top, runtime_height)
        m2_y = y_scale_log(row["m2_total_sec"], runtime_min, runtime_max, runtime_top, runtime_height)
        cpp_runtime_pts.append((x, cpp_y))
        m2_runtime_pts.append((x, m2_y))
        parts.append(circle(x, cpp_y, cpp_color))
        parts.append(circle(x, m2_y, m2_color))
        parts.append(text(x, runtime_top + runtime_height + 22, f"d={row['max_degree']}", 12, anchor="middle"))
        parts.append(text(x, runtime_top + runtime_height + 40, f"n={row['n']:,}", 11, anchor="middle"))
    parts.append(polyline(cpp_runtime_pts, cpp_color))
    parts.append(polyline(m2_runtime_pts, m2_color))

    parts.append(text(plot_left + panel_width / 2, runtime_top + runtime_height + 68, "max degree / number of tested degree sequences", 13, anchor="middle"))

    parts.append(text(40, 460, "Peak memory (maxrss_kb, linear scale)", 18, weight="bold"))
    parts.append(rect(plot_left, memory_top, panel_width, memory_height, "#ffffff", "#d1d5db"))
    for tick in mem_ticks:
        y = y_scale_linear(tick, mem_min, mem_max, memory_top, memory_height)
        parts.append(line(plot_left, y, plot_right, y, grid_color, 1))
        parts.append(text(plot_left - 12, y + 5, f"{int(tick):,}", 12, anchor="end"))

    cpp_mem_pts = []
    m2_mem_pts = []
    for row in rows:
        x = x_scale(row["n"], min_n, max_n, plot_left, panel_width)
        cpp_y = y_scale_linear(row["cpp_mem_kb"], mem_min, mem_max, memory_top, memory_height)
        m2_y = y_scale_linear(row["m2_mem_kb"], mem_min, mem_max, memory_top, memory_height)
        cpp_mem_pts.append((x, cpp_y))
        m2_mem_pts.append((x, m2_y))
        parts.append(circle(x, cpp_y, cpp_color))
        parts.append(circle(x, m2_y, m2_color))
        parts.append(text(x, memory_top + memory_height + 22, f"d={row['max_degree']}", 12, anchor="middle"))
        parts.append(text(x, memory_top + memory_height + 40, f"n={row['n']:,}", 11, anchor="middle"))
    parts.append(polyline(cpp_mem_pts, cpp_color))
    parts.append(polyline(m2_mem_pts, m2_color))

    parts.append(text(plot_left + panel_width / 2, memory_top + memory_height + 68, "max degree / number of tested degree sequences", 13, anchor="middle"))
    parts.append(text(40, 815, "Generated with scripts/generate_benchmark_svg.py", 12))
    parts.append("</svg>")

    out_path.write_text("\n".join(parts), encoding="utf-8")


def main():
    if len(sys.argv) != 4:
        print("Usage: python3 scripts/generate_benchmark_svg.py <csv_path> <codim> <output_svg>", file=sys.stderr)
        return 1

    csv_path = Path(sys.argv[1])
    codim = int(sys.argv[2])
    out_path = Path(sys.argv[3])
    rows = read_rows(csv_path, codim)
    if not rows:
        print(f"No rows found for codimension {codim}", file=sys.stderr)
        return 2
    out_path.parent.mkdir(parents=True, exist_ok=True)
    generate_svg(rows, codim, out_path)
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
