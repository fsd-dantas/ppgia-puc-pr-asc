#!/usr/bin/env python3
"""
plot_inversion.py — Timeline de Inversão de Prioridade: GPOS vs RTOS

Lê CSVs de eventos e gera um diagrama de swim-lane comparando:
  - Cenário sem PIP (k_sem / PTHREAD_PRIO_NONE)
  - Cenário com PIP (k_mutex / PTHREAD_PRIO_INHERIT)

Formatos de entrada suportados:

  Linux (dados/linux/priority_inversion.csv):
    scenario,task,event,wall_us
    no_pip,T3,"adquiriu S1 ...",10234

  RTOS / Zephyr (dados/rtos/priority_inversion.csv):
    INV,10234,T3,adquiriu S1 - iniciando trabalho pesado

Uso:
  python scripts/plot_inversion.py
  python scripts/plot_inversion.py --linux dados/linux/priority_inversion.csv
"""

import argparse
import re
import sys
from pathlib import Path

import matplotlib
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd

matplotlib.rcParams.update({
    "font.family": "monospace",
    "axes.titlesize": 11,
    "axes.labelsize": 10,
    "xtick.labelsize": 8,
    "ytick.labelsize": 9,
    "figure.dpi": 150,
})

SCRIPT_DIR = Path(__file__).parent
DATA_DIR   = SCRIPT_DIR.parent / "dados"

DEFAULT_LINUX = DATA_DIR / "linux" / "priority_inversion.csv"
DEFAULT_RTOS  = DATA_DIR / "rtos"  / "priority_inversion.csv"
DEFAULT_OUT   = DATA_DIR / "timeline_inversao.png"

# ── Paleta ─────────────────────────────────────────────────────
COLOR = {
    "T1_running":  "#27ae60",   # verde
    "T1_blocked":  "#e74c3c",   # vermelho — bloqueada aguardando S1
    "T2_running":  "#2980b9",   # azul
    "T3_running":  "#95a5a6",   # cinza — correndo sem S1
    "T3_holding":  "#e67e22",   # laranja — segurando S1
    "idle":        "#ecf0f1",   # quase branco
}

LANE_H   = 0.55   # altura de cada barra swim-lane
LANE_GAP = 1.0    # espaçamento entre lanes

# ── Parser — Linux CSV ─────────────────────────────────────────
def load_linux(path: Path) -> pd.DataFrame | None:
    if not path.exists():
        return None
    df = pd.read_csv(path)
    # colunas esperadas: scenario, task, event, wall_us
    if not {"scenario", "task", "event", "wall_us"}.issubset(df.columns):
        print(f"Aviso: {path} tem colunas inesperadas", file=sys.stderr)
        return None
    df["env"] = "Linux"
    df["wall_us"] = df["wall_us"].astype(float)
    return df[["env", "scenario", "task", "event", "wall_us"]]


# ── Parser — RTOS CSV (linhas "INV,us,task,event") ────────────
def load_rtos(path: Path) -> pd.DataFrame | None:
    if not path.exists():
        return None

    rows = []
    scenario = None

    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                # detecta marcador de cenário nos comentários
                if "no_pip" in line:
                    scenario = "no_pip"
                elif "pip" in line and "no_pip" not in line:
                    scenario = "pip"
                continue

            parts = line.split(",", 3)
            if len(parts) < 4 or parts[0] != "INV":
                continue

            _, wall_us, task, event = parts
            try:
                rows.append({
                    "env":      "RTOS",
                    "scenario": scenario or "unknown",
                    "task":     task.strip(),
                    "event":    event.strip(),
                    "wall_us":  float(wall_us),
                })
            except ValueError:
                continue

    if not rows:
        return None
    return pd.DataFrame(rows)


# ── Classificação de eventos ───────────────────────────────────
def classify(task: str, event: str) -> str:
    """Retorna um tag semântico para o evento."""
    e = event.lower()
    if task == "T3":
        if re.search(r"adquiriu.+s1", e):
            return "T3_ACQUIRED"
        if re.search(r"(liberando|conclu)", e):
            return "T3_RELEASING"
        if "fim" in e:
            return "T3_DONE"
        if "tentando" in e or "adquirindo" in e:
            return "T3_TRYING"
    elif task == "T1":
        if "tentando" in e:
            return "T1_TRYING"
        if re.search(r"adquiriu.+s1", e):
            return "T1_ACQUIRED"
        if "fim" in e:
            return "T1_DONE"
    elif task == "T2":
        if "iniciou" in e:
            return "T2_START"
        if "fim" in e:
            return "T2_DONE"
    return "OTHER"


def extract_key_times(df: pd.DataFrame) -> dict:
    """
    A partir dos eventos de um único cenário (no_pip ou pip),
    extrai instantes-chave normalizados para t=0 no primeiro evento.
    """
    t0 = df["wall_us"].min()
    times = {}

    for _, row in df.iterrows():
        tag = classify(row["task"], row["event"])
        us  = row["wall_us"] - t0
        if tag not in times:
            times[tag] = us

    return times, t0


# ── Desenha um swim-lane para um cenário ──────────────────────
def draw_swimlane(ax: plt.Axes, df: pd.DataFrame, title: str) -> None:
    """
    Swim-lane de T1 (lane y=2), T2 (y=1), T3 (y=0).
    Barras coloridas representam estados inferidos dos eventos.
    """
    times, t0 = extract_key_times(df)

    # Converte para ms para melhor legibilidade
    def ms(key: str, fallback: float = None) -> float | None:
        v = times.get(key, fallback)
        return v / 1_000 if v is not None else fallback

    t_end = (df["wall_us"].max() - t0) / 1_000 * 1.05  # margem 5%

    LANES = {"T3": 0, "T2": 1, "T1": 2}

    def bar(task, x_start, x_end, color, alpha=0.85, hatch="", label=None):
        if x_start is None or x_end is None or x_end <= x_start:
            return
        y = LANES[task] - LANE_H / 2
        ax.broken_barh(
            [(x_start, x_end - x_start)],
            (y, LANE_H),
            facecolors=color,
            alpha=alpha,
            edgecolor="white",
            linewidth=0.4,
            hatch=hatch,
        )

    # ── T3: idle → holding S1 → idle ─────────────────────────
    t3_acquired  = ms("T3_ACQUIRED")
    t3_releasing = ms("T3_RELEASING")
    t3_done      = ms("T3_DONE", t_end)

    bar("T3", 0,            t3_acquired,  COLOR["T3_running"])
    bar("T3", t3_acquired,  t3_releasing, COLOR["T3_holding"], hatch="///",
        label="T3 segurando S1")
    bar("T3", t3_releasing, t3_done,      COLOR["T3_running"])

    # ── T2: idle → working ────────────────────────────────────
    t2_start = ms("T2_START")
    t2_done  = ms("T2_DONE", t_end)

    if t2_start is not None:
        bar("T2", 0,        t2_start, COLOR["idle"])
        bar("T2", t2_start, t2_done,  COLOR["T2_running"], hatch="\\\\")

    # ── T1: idle → blocked (vermelho) → running ───────────────
    t1_trying   = ms("T1_TRYING")
    t1_acquired = ms("T1_ACQUIRED")
    t1_done     = ms("T1_DONE", t_end)

    if t1_trying is not None:
        bar("T1", 0,           t1_trying,   COLOR["idle"])
        bar("T1", t1_trying,   t1_acquired, COLOR["T1_blocked"], alpha=0.9,
            label="T1 bloqueada (inversão)")
        bar("T1", t1_acquired, t1_done,     COLOR["T1_running"])

    # ── Linhas de evento ──────────────────────────────────────
    for tag, col, ls, lbl in [
        ("T3_ACQUIRED",  COLOR["T3_holding"], "--", "T3 adquiriu S1"),
        ("T3_RELEASING", "#e67e22",           "-.", "T3 liberou S1"),
        ("T1_TRYING",    COLOR["T1_blocked"], ":",  "T1 tenta S1"),
        ("T1_ACQUIRED",  COLOR["T1_running"], "--", "T1 adquiriu S1"),
        ("T2_START",     COLOR["T2_running"], ":",  "T2 inicia"),
    ]:
        t = ms(tag)
        if t is not None:
            ax.axvline(t, color=col, linestyle=ls, linewidth=0.8, alpha=0.7)

    # ── Anotação de bloqueio de T1 ────────────────────────────
    if t1_trying is not None and t1_acquired is not None:
        block_ms = t1_acquired - t1_trying
        mid_x    = (t1_trying + t1_acquired) / 2
        y_t1     = LANES["T1"]
        ax.annotate(
            f"⬛ T1 bloqueada\n{block_ms:.1f} ms",
            xy=(mid_x, y_t1),
            fontsize=7.5,
            ha="center", va="center",
            color="white",
            fontweight="bold",
        )

    # ── Formatação dos eixos ──────────────────────────────────
    ax.set_yticks(list(LANES.values()))
    ax.set_yticklabels(list(LANES.keys()), fontweight="bold")
    ax.set_xlim(0, t_end)
    ax.set_xlabel("Tempo (ms)")
    ax.set_title(title, pad=5, fontweight="bold")
    ax.xaxis.set_minor_locator(ticker.AutoMinorLocator())
    ax.grid(axis="x", linestyle="--", alpha=0.3)
    ax.set_ylim(-0.6, 2.6)


# ── Gráfico de barras: tempo de bloqueio de T1 ────────────────
def draw_block_comparison(ax: plt.Axes,
                           envs_no_pip: list[tuple[str, pd.DataFrame]],
                           envs_pip:    list[tuple[str, pd.DataFrame]]) -> None:
    """
    Barras agrupadas: T1 block time sem PIP vs com PIP, por ambiente.
    """
    labels, no_pip_vals, pip_vals = [], [], []

    all_pairs = {}
    for label, df in envs_no_pip:
        all_pairs.setdefault(label, {})["no_pip"] = df
    for label, df in envs_pip:
        all_pairs.setdefault(label, {})["pip"] = df

    for env_label, pair in all_pairs.items():
        for scenario_key, color in [("no_pip", "#e74c3c"), ("pip", "#27ae60")]:
            if scenario_key not in pair:
                continue
            df = pair[scenario_key]
            times, _ = extract_key_times(df)
            t_try = times.get("T1_TRYING")
            t_got = times.get("T1_ACQUIRED")
            block_ms = (t_got - t_try) / 1_000 if t_try and t_got else 0
            labels.append(f"{env_label}\n({'sem' if scenario_key == 'no_pip' else 'com'} PIP)")
            no_pip_vals.append(block_ms if scenario_key == "no_pip" else 0)
            pip_vals.append(block_ms if scenario_key == "pip" else 0)

    # Simplifica: uma barra por cenário
    x = np.arange(len(labels))
    vals = [v if v > 0 else p for v, p in zip(no_pip_vals, pip_vals)]
    colors = ["#e74c3c" if "sem" in l else "#27ae60" for l in labels]

    bars = ax.bar(x, vals, color=colors, alpha=0.85, edgecolor="white", linewidth=0.5)

    for bar_patch, v in zip(bars, vals):
        if v > 0:
            ax.text(
                bar_patch.get_x() + bar_patch.get_width() / 2,
                bar_patch.get_height() + max(vals) * 0.01,
                f"{v:.1f} ms",
                ha="center", va="bottom", fontsize=8, fontweight="bold",
            )

    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=8)
    ax.set_ylabel("T1 tempo bloqueada (ms)")
    ax.set_title("Comparação: Tempo de Bloqueio de T1", fontweight="bold")
    ax.grid(axis="y", linestyle="--", alpha=0.35)
    ax.set_ylim(0, max(vals) * 1.25 if vals else 1)


# ── Composição da figura ───────────────────────────────────────
def plot_all(datasets: list[tuple[str, pd.DataFrame]], out_path: Path) -> None:
    """
    datasets: [(env_label, df), ...]  — cada df contém múltiplos cenários.
    """
    # Separa cenários
    rows_swimlane = []  # (title, df_scenario)
    rows_no_pip, rows_pip = [], []

    for env_label, df in datasets:
        for scenario in ["no_pip", "pip"]:
            sub = df[df["scenario"] == scenario].copy()
            if sub.empty:
                continue
            label_scenario = "sem PIP (inversão)" if scenario == "no_pip" \
                             else "com PIP (resolvido)"
            title = f"{env_label} — {label_scenario}"
            rows_swimlane.append((title, sub))
            if scenario == "no_pip":
                rows_no_pip.append((env_label, sub))
            else:
                rows_pip.append((env_label, sub))

    n_rows = len(rows_swimlane)
    if n_rows == 0:
        print("Nenhum dado de inversão encontrado.", file=sys.stderr)
        sys.exit(1)

    # +1 linha para gráfico de comparação
    fig, axes = plt.subplots(
        n_rows + 1, 1,
        figsize=(11, 3.5 * n_rows + 3.5),
        gridspec_kw={"height_ratios": [1] * n_rows + [0.8]},
    )
    if n_rows + 1 == 1:
        axes = [axes]

    fig.suptitle(
        "Análise de Inversão de Prioridade\nTimeline de Execução das Tarefas",
        fontsize=14, fontweight="bold",
    )

    # Swim-lanes
    for i, (title, df_s) in enumerate(rows_swimlane):
        draw_swimlane(axes[i], df_s, title)

    # Legenda unificada
    legend_patches = [
        mpatches.Patch(color=COLOR["T1_blocked"],  label="T1 bloqueada (aguardando S1)"),
        mpatches.Patch(color=COLOR["T1_running"],  label="T1 em execução"),
        mpatches.Patch(color=COLOR["T2_running"],  label="T2 em execução"),
        mpatches.Patch(color=COLOR["T3_holding"],  label="T3 segurando S1"),
        mpatches.Patch(color=COLOR["T3_running"],  label="T3 em execução (sem S1)"),
        mpatches.Patch(color=COLOR["idle"],        label="idle"),
    ]
    axes[0].legend(
        handles=legend_patches,
        loc="lower right",
        fontsize=7,
        ncol=2,
        framealpha=0.85,
    )

    # Barra de comparação
    draw_block_comparison(axes[-1], rows_no_pip, rows_pip)

    fig.tight_layout(rect=[0, 0, 1, 0.97])
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, bbox_inches="tight")
    print(f"Timeline salva em: {out_path}")

    # Resumo terminal
    print("\n── Tempo de bloqueio de T1 ──────────────────────────")
    for env_label, df in datasets:
        print(f"\n  {env_label}")
        for scenario in ["no_pip", "pip"]:
            sub = df[df["scenario"] == scenario]
            if sub.empty:
                continue
            times, _ = extract_key_times(sub)
            t_try = times.get("T1_TRYING")
            t_got = times.get("T1_ACQUIRED")
            if t_try and t_got:
                block_ms = (t_got - t_try) / 1_000
                label_s  = "sem PIP" if scenario == "no_pip" else "com PIP"
                print(f"    {label_s:10s}: {block_ms:7.1f} ms")
    print()


# ── CLI ────────────────────────────────────────────────────────
def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--linux", type=Path, default=DEFAULT_LINUX)
    parser.add_argument("--rtos",  type=Path, default=DEFAULT_RTOS)
    parser.add_argument("--out",   type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()

    datasets = []

    linux_df = load_linux(args.linux)
    if linux_df is not None:
        datasets.append(("Linux GPOS", linux_df))
    else:
        print(f"Aviso: {args.linux} não encontrado.", file=sys.stderr)

    rtos_df = load_rtos(args.rtos)
    if rtos_df is not None:
        datasets.append(("RTOS (Zephyr)", rtos_df))
    else:
        print(f"Aviso: {args.rtos} não encontrado.", file=sys.stderr)

    if not datasets:
        print("Erro: nenhum dado de inversão disponível.", file=sys.stderr)
        sys.exit(1)

    plot_all(datasets, args.out)


if __name__ == "__main__":
    main()
