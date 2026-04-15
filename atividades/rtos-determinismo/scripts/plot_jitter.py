#!/usr/bin/env python3
"""
plot_jitter.py — Histograma de Jitter: Linux GPOS vs RTOS

Lê dados/linux/jitter.csv (obrigatório) e dados/rtos/jitter.csv (opcional).
Gera dados/histograma_jitter.png com:
  - Histograma de frequência para cada ambiente
  - Linhas verticais: média, P99, P99.9
  - Tabela de estatísticas integrada na figura
  - Eixo X em microssegundos

Uso:
  python scripts/plot_jitter.py
  python scripts/plot_jitter.py --linux dados/linux/jitter.csv --rtos dados/rtos/jitter.csv
"""

import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

matplotlib.rcParams.update({
    "font.family": "monospace",
    "axes.titlesize": 13,
    "axes.labelsize": 11,
    "xtick.labelsize": 9,
    "ytick.labelsize": 9,
    "figure.dpi": 150,
})

# ── Caminhos padrão ────────────────────────────────────────────
SCRIPT_DIR  = Path(__file__).parent
DATA_DIR    = SCRIPT_DIR.parent / "dados"
OUT_PATH    = DATA_DIR / "histograma_jitter.png"

DEFAULT_LINUX = DATA_DIR / "linux" / "jitter.csv"
DEFAULT_RTOS  = DATA_DIR / "rtos"  / "jitter.csv"


# ── Carregamento e validação ───────────────────────────────────
def load_jitter(csv_path: Path) -> np.ndarray | None:
    if not csv_path.exists():
        return None
    df = pd.read_csv(csv_path)
    if "jitter_ns" not in df.columns:
        print(f"Aviso: {csv_path} não tem coluna 'jitter_ns'", file=sys.stderr)
        return None
    return df["jitter_ns"].to_numpy(dtype=float) / 1_000  # ns → µs


# ── Estatísticas ───────────────────────────────────────────────
def compute_stats(data: np.ndarray) -> dict:
    return {
        "n":       len(data),
        "mean":    np.mean(data),
        "std":     np.std(data),
        "min":     np.min(data),
        "max":     np.max(data),
        "p50":     np.percentile(data, 50),
        "p95":     np.percentile(data, 95),
        "p99":     np.percentile(data, 99),
        "p999":    np.percentile(data, 99.9),
    }


# ── Plot ───────────────────────────────────────────────────────
def plot(linux_data: np.ndarray | None, rtos_data: np.ndarray | None,
         out_path: Path) -> None:

    datasets = []
    if linux_data is not None:
        datasets.append(("Linux GPOS (SCHED_FIFO)", linux_data, "#e74c3c", "///"))
    if rtos_data is not None:
        datasets.append(("RTOS (Zephyr/QEMU)", rtos_data, "#2980b9", r"\\\\"))

    if not datasets:
        print("Erro: nenhum dado disponível.", file=sys.stderr)
        sys.exit(1)

    fig, axes = plt.subplots(
        len(datasets), 1,
        figsize=(10, 4.5 * len(datasets)),
        squeeze=False,
    )
    fig.suptitle(
        "Caracterização de Jitter — T1 (período 100 ms)\nGPOS vs RTOS sob estresse de CPU",
        fontsize=14, fontweight="bold", y=1.01,
    )

    for row, (label, data, color, hatch) in enumerate(datasets):
        ax = axes[row][0]
        stats = compute_stats(data)

        # ── Histograma ─────────────────────────────────────────
        bins = min(80, max(20, len(data) // 12))
        counts, edges, patches = ax.hist(
            data, bins=bins,
            color=color, alpha=0.70,
            edgecolor="white", linewidth=0.4,
            hatch=hatch, label=label,
        )

        # ── Linhas estatísticas ────────────────────────────────
        for pct_label, val, ls in [
            ("média",  stats["mean"], "--"),
            ("P99",    stats["p99"],  "-."),
            ("P99.9",  stats["p999"], ":"),
        ]:
            ax.axvline(val, color="black", linestyle=ls, linewidth=1.2,
                       label=f"{pct_label} = {val:.1f} µs")

        # ── Tabela de estatísticas ─────────────────────────────
        cell_text = [
            ["N",       f"{stats['n']:,}"],
            ["Média",   f"{stats['mean']:.2f} µs"],
            ["StdDev",  f"{stats['std']:.2f} µs"],
            ["Min",     f"{stats['min']:.2f} µs"],
            ["Max",     f"{stats['max']:.2f} µs"],
            ["P50",     f"{stats['p50']:.2f} µs"],
            ["P95",     f"{stats['p95']:.2f} µs"],
            ["P99",     f"{stats['p99']:.2f} µs"],
            ["P99.9",   f"{stats['p999']:.2f} µs"],
        ]
        table = ax.table(
            cellText=cell_text,
            colLabels=["Stat", "Valor"],
            loc="upper right",
            cellLoc="left",
            bbox=[0.72, 0.30, 0.27, 0.68],
        )
        table.auto_set_font_size(False)
        table.set_fontsize(8)
        for (r, c), cell in table.get_celld().items():
            cell.set_edgecolor("#cccccc")
            if r == 0:
                cell.set_facecolor("#dddddd")
                cell.set_text_props(fontweight="bold")
            else:
                cell.set_facecolor("#f9f9f9" if r % 2 == 0 else "white")

        # ── Cauda longa — destaque visual ─────────────────────
        tail_threshold = stats["p99"]
        tail_mask = data >= tail_threshold
        if tail_mask.any():
            ax.hist(
                data[tail_mask], bins=bins,
                color="#f39c12", alpha=0.85,
                edgecolor="darkorange", linewidth=0.5,
                label=f"cauda longa (≥ P99, n={tail_mask.sum()})",
            )

        # ── Formatação ─────────────────────────────────────────
        ax.set_title(label, fontweight="bold", pad=6)
        ax.set_xlabel("Jitter (µs)")
        ax.set_ylabel("Frequência")
        ax.xaxis.set_major_formatter(ticker.FormatStrFormatter("%.0f µs"))
        ax.grid(axis="y", linestyle="--", alpha=0.4)
        ax.legend(fontsize=8, loc="upper left")

    fig.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, bbox_inches="tight")
    print(f"Histograma salvo em: {out_path}")

    # ── Resumo no terminal ─────────────────────────────────────
    print("\n── Estatísticas ──────────────────────────────────")
    for label, data, *_ in datasets:
        s = compute_stats(data)
        print(f"\n  {label}")
        print(f"    N        : {s['n']:,}")
        print(f"    Média    : {s['mean']:.2f} µs")
        print(f"    StdDev   : {s['std']:.2f} µs")
        print(f"    Min/Max  : {s['min']:.2f} / {s['max']:.2f} µs")
        print(f"    P99      : {s['p99']:.2f} µs")
        print(f"    P99.9    : {s['p999']:.2f} µs")
    print()


# ── CLI ────────────────────────────────────────────────────────
def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--linux", type=Path, default=DEFAULT_LINUX,
                        help="CSV de jitter Linux (padrão: dados/linux/jitter.csv)")
    parser.add_argument("--rtos",  type=Path, default=DEFAULT_RTOS,
                        help="CSV de jitter RTOS  (padrão: dados/rtos/jitter.csv)")
    parser.add_argument("--out",   type=Path, default=OUT_PATH,
                        help="Caminho de saída do PNG")
    args = parser.parse_args()

    linux_data = load_jitter(args.linux)
    rtos_data  = load_jitter(args.rtos)

    if linux_data is None and rtos_data is None:
        print("Erro: nenhum CSV encontrado.", file=sys.stderr)
        print(f"  Esperado: {args.linux} e/ou {args.rtos}", file=sys.stderr)
        sys.exit(1)

    if linux_data is None:
        print(f"Aviso: {args.linux} não encontrado — apenas RTOS.")
    if rtos_data is None:
        print(f"Aviso: {args.rtos} não encontrado — apenas Linux.")

    plot(linux_data, rtos_data, args.out)


if __name__ == "__main__":
    main()
