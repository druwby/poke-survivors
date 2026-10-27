#!/usr/bin/env python3
"""
tools/ml_optimizer.py
Machine Learning & Big Data Evolutionary Parameter Balancing Engine

Runs parallel headless C simulations across CPU cores to find the optimal numerical balance
for enemy stats, scaling formulas, defense ratings, and spawn densities.

Objectives:
- Eliminates enemy instant-melting (enforces Stage 1 TTK target: 2.0s - 4.5s)
- Establishes clear agency/skill gap (Bot Optimal win rate 75-85% vs Bot Mediocre 20-35%)
- Minimizes excessive overkill waste
- Maintains combat tension (danger zone HP engagement)
"""

import argparse
import copy
import json
import math
import multiprocessing
import os
import random
import subprocess
import sys
import tempfile
import time
from typing import Dict, Any, List, Tuple

# Numerical Parameter Boundaries for Optimization
PARAM_SPECS = {
    "enemy_hp_mult":          {"min": 1.20, "max": 3.50, "default": 2.20, "step": 0.05},
    "route_hp_scaling":       {"min": 0.60, "max": 2.20, "default": 1.00, "step": 0.05},
    "time_hp_alpha":          {"min": 0.05, "max": 0.60, "default": 0.15, "step": 0.02},
    "time_hp_gamma":          {"min": 0.60, "max": 1.80, "default": 1.00, "step": 0.05},
    "enemy_defense_base":     {"min": 0.05, "max": 0.35, "default": 0.12, "step": 0.02},
    "enemy_defense_per_stage":{"min": 0.02, "max": 0.16, "default": 0.08, "step": 0.01},
    "weapon_damage_mult":     {"min": 0.70, "max": 1.30, "default": 1.00, "step": 0.05},
    "spawn_interval_mult":    {"min": 0.60, "max": 1.60, "default": 1.00, "step": 0.05},
    "max_enemies_mult":       {"min": 0.70, "max": 1.50, "default": 1.00, "step": 0.05},
    "boss_hp_mult":           {"min": 0.80, "max": 2.50, "default": 1.00, "step": 0.05},
}


def sample_random_candidate() -> Dict[str, float]:
    cand = {}
    for param, spec in PARAM_SPECS.items():
        val = random.uniform(spec["min"], spec["max"])
        cand[param] = round(val, 3)
    return cand


def mutate_candidate(cand: Dict[str, float], mutation_rate: float = 0.30, sigma: float = 0.15) -> Dict[str, float]:
    mutated = copy.deepcopy(cand)
    for param, spec in PARAM_SPECS.items():
        if random.random() < mutation_rate:
            span = spec["max"] - spec["min"]
            delta = random.gauss(0.0, sigma * span)
            new_val = mutated[param] + delta
            new_val = max(spec["min"], min(spec["max"], new_val))
            mutated[param] = round(new_val, 3)
    return mutated


def crossover_candidates(parent_a: Dict[str, float], parent_b: Dict[str, float]) -> Dict[str, float]:
    child = {}
    for param in PARAM_SPECS.keys():
        if random.random() < 0.5:
            child[param] = parent_a[param]
        else:
            child[param] = parent_b[param]
    return child


def run_c_simulation(bin_path: str, config: Dict[str, float], archetype: str, trials: int, sim_time: float) -> Dict[str, Any]:
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False) as f:
        tmp_name = f.name
        json.dump(config, f)

    try:
        cmd = [
            bin_path,
            "--simulate", str(trials),
            "--json-output",
            "--config-override", tmp_name,
            "--bot-archetype", archetype,
            "--sim-time", str(sim_time)
        ]
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=120)
        if proc.returncode != 0:
            return {}
        
        # Locate JSON content in stdout
        stdout_txt = proc.stdout.strip()
        start_idx = stdout_txt.find('{')
        end_idx = stdout_txt.rfind('}')
        if start_idx == -1 or end_idx == -1:
            return {}
        clean_json = stdout_txt[start_idx:end_idx + 1]
        return json.loads(clean_json)
    except Exception as e:
        return {}
    finally:
        if os.path.exists(tmp_name):
            try:
                os.remove(tmp_name)
            except OSError:
                pass


def evaluate_candidate(args_tuple) -> Tuple[Dict[str, float], float, Dict[str, Any]]:
    cand, bin_path, trials, sim_time = args_tuple

    # Evaluate with Mediocre bot
    res_mediocre = run_c_simulation(bin_path, cand, "mediocre", trials, sim_time)
    # Evaluate with Optimal bot
    res_optimal = run_c_simulation(bin_path, cand, "optimal", trials, sim_time)

    if not res_mediocre or not res_optimal:
        return cand, -9999.0, {}

    # Calculate multi-objective loss components
    loss = 0.0

    # 1. Stage 1 TTK Loss (Target 2.0s - 3.5s)
    ttk_s1_med = res_mediocre.get("overall_avg_ttk_stage1", 0.0)
    ttk_s1_opt = res_optimal.get("overall_avg_ttk_stage1", 0.0)
    avg_ttk = (ttk_s1_med + ttk_s1_opt) / 2.0

    if avg_ttk < 2.0:
        # Severe penalty for paper-thin enemies dying too fast!
        loss += (2.0 - avg_ttk) * 100.0
    elif avg_ttk > 3.5:
        # Penalty for bullet sponges
        loss += (avg_ttk - 3.5) * 60.0

    # Starter Parity: Penalize any starter with extreme TTK divergence
    for s in res_optimal.get("starters", []):
        stk = s.get("avg_ttk_stage1", 0.0)
        if stk < 1.2:
            loss += (1.2 - stk) * 50.0
        elif stk > 4.5:
            loss += (stk - 4.5) * 40.0

    # 2. Instant Kills (<1.0s) Penalty
    instant_kills = res_mediocre.get("total_instant_kills", 0) + res_optimal.get("total_instant_kills", 0)
    loss += instant_kills * 5.0

    # 3. Mediocre Bot Win Rate (Target 20% - 35%)
    win_med = res_mediocre.get("overall_survival_rate", 0.0)
    if win_med > 45.0:
        loss += (win_med - 45.0) * 4.0
    elif win_med < 15.0:
        loss += (15.0 - win_med) * 3.0

    # 4. Optimal Bot Win Rate (Target 75% - 90%)
    win_opt = res_optimal.get("overall_survival_rate", 0.0)
    if win_opt < 65.0:
        loss += (65.0 - win_opt) * 4.5
    elif win_opt > 95.0:
        loss += (win_opt - 95.0) * 2.0

    # 5. Skill Gap Bonus (Optimal win rate should comfortably exceed Mediocre win rate)
    skill_gap = win_opt - win_med
    if skill_gap < 30.0:
        loss += (30.0 - skill_gap) * 5.0

    # 6. Overkill Ratio Penalty (Ideal: 5% - 20%)
    overkill_med = 0.0
    overkill_opt = 0.0
    for s in res_mediocre.get("starters", []):
        overkill_med += s.get("overkill_ratio_pct", 0.0)
    for s in res_optimal.get("starters", []):
        overkill_opt += s.get("overkill_ratio_pct", 0.0)
    avg_overkill = (overkill_med + overkill_opt) / 6.0
    if avg_overkill > 25.0:
        loss += (avg_overkill - 25.0) * 2.5

    fitness = 1000.0 - loss

    metrics = {
        "avg_ttk_stage1": round(avg_ttk, 2),
        "instant_kills": instant_kills,
        "win_rate_mediocre": round(win_med, 1),
        "win_rate_optimal": round(win_opt, 1),
        "skill_gap": round(skill_gap, 1),
        "avg_overkill_pct": round(avg_overkill, 2),
        "loss": round(loss, 2)
    }

    return cand, fitness, metrics


def optimize(bin_path: str, pop_size: int, generations: int, trials: int, sim_time: float, cores: int, output_path: str):
    print("=" * 75)
    print(" MACHINE LEARNING & BIG DATA EVOLUTIONARY BALANCE OPTIMIZER")
    print("=" * 75)
    print(f" Binary Executable : {bin_path}")
    print(f" Population Size   : {pop_size}")
    print(f" Generations       : {generations}")
    print(f" Trials Per Eval   : {trials} per starter (x3 starters x2 archetypes = {trials*6} sims/cand)")
    print(f" Sim Time Limit    : {sim_time}s")
    print(f" CPU Worker Cores  : {cores}")
    print(f" Best Config Output: {output_path}")
    print("=" * 75)

    # Initialize population: 1 default config + random candidates
    default_cand = {p: s["default"] for p, s in PARAM_SPECS.items()}
    population = [default_cand]
    while len(population) < pop_size:
        population.append(sample_random_candidate())

    best_cand = None
    best_fitness = -float("inf")
    best_metrics = {}

    start_time = time.time()

    with multiprocessing.Pool(processes=cores) as pool:
        for gen in range(1, generations + 1):
            gen_start = time.time()
            print(f"\n>>> GENERATION {gen}/{generations} (Evaluating {len(population)} candidates)...")

            tasks = [(cand, bin_path, trials, sim_time) for cand in population]
            results = pool.map(evaluate_candidate, tasks)

            # Sort by fitness descending
            results.sort(key=lambda x: x[1], reverse=True)

            gen_best_cand, gen_best_fitness, gen_best_metrics = results[0]
            avg_fitness = sum(r[1] for r in results) / len(results)

            if gen_best_fitness > best_fitness:
                best_fitness = gen_best_fitness
                best_cand = gen_best_cand
                best_metrics = gen_best_metrics
                # Save checkpoint immediately
                with open(output_path, "w") as f:
                    json.dump(best_cand, f, indent=2)

            gen_dur = time.time() - gen_start
            print(f"  [Gen {gen:02d} Summary] Best Fit: {gen_best_fitness:.1f} | Avg Fit: {avg_fitness:.1f} | Duration: {gen_dur:.1f}s")
            print(f"     -> Best Metrics: TTK S1: {gen_best_metrics.get('avg_ttk_stage1', 0)}s | "
                  f"WinRate Med: {gen_best_metrics.get('win_rate_mediocre', 0)}% | "
                  f"WinRate Opt: {gen_best_metrics.get('win_rate_optimal', 0)}% | "
                  f"SkillGap: {gen_best_metrics.get('skill_gap', 0)}% | "
                  f"InstaKills: {gen_best_metrics.get('instant_kills', 0)}")

            # Breeding next generation (if not last gen)
            if gen < generations:
                new_pop = []
                # Elitism: retain top 2 candidates unchanged
                new_pop.append(results[0][0])
                if len(results) > 1:
                    new_pop.append(results[1][0])

                # Tournament selection pool (top 50%)
                survivors = [r[0] for r in results[:max(2, pop_size // 2)]]

                while len(new_pop) < pop_size:
                    parent_a = random.choice(survivors)
                    parent_b = random.choice(survivors)
                    child = crossover_candidates(parent_a, parent_b)
                    child = mutate_candidate(child, mutation_rate=0.30, sigma=0.15)
                    new_pop.append(child)

                population = new_pop

    total_time = time.time() - start_time
    print("\n" + "=" * 75)
    print(" OPTIMIZATION COMPLETED SUCCESSFULLY")
    print("=" * 75)
    print(f" Total Wall Time : {total_time:.1f}s")
    print(f" Best Fitness    : {best_fitness:.2f}")
    print(f" Final Metrics   : {json.dumps(best_metrics, indent=2)}")
    print(f" Best Parameters : {json.dumps(best_cand, indent=2)}")
    print(f" Saved To        : {output_path}")
    print("=" * 75)


def main():
    parser = argparse.ArgumentParser(description="Evolutionary Balance Optimizer for Rogue Pokemon Game")
    parser.add_argument("--bin", default="./rogue_game", help="Path to compiled rogue_game binary")
    parser.add_argument("--pop-size", type=int, default=12, help="Population size per generation")
    parser.add_argument("--generations", type=int, default=5, help="Number of evolutionary generations")
    parser.add_argument("--trials", type=int, default=3, help="Simulation trials per starter")
    parser.add_argument("--sim-time", type=float, default=60.0, help="Simulated seconds per trial")
    parser.add_argument("--cores", type=int, default=max(1, multiprocessing.cpu_count() - 1), help="CPU cores for parallel execution")
    parser.add_argument("--output", default="best_balance_config.json", help="Path to save best JSON balance config")

    args = parser.parse_args()

    if not os.path.exists(args.bin):
        print(f"Error: Binary '{args.bin}' not found. Please compile first with 'make'.", file=sys.stderr)
        sys.exit(1)

    optimize(args.bin, args.pop_size, args.generations, args.trials, args.sim_time, args.cores, args.output)


if __name__ == "__main__":
    main()
