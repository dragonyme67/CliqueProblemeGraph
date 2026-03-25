#pragma once

#include "graph.hpp"
#include <vector>

/**
 * Algorithmes de Hill Climbing pour le problème de la Clique Maximum.
 * Chaque stratégie est implémentée indépendamment.
 */

// --- Stratégies N1 (1 opération) ---
std::vector<vertex> solve_n1_best(const Graph &g, vertex v);

// --- Stratégies N2 (2 opérations) ---
std::vector<vertex> solve_n2_add_best(const Graph &g, vertex v);
std::vector<vertex> solve_n2_add_first(const Graph &g, vertex v);
std::vector<vertex> solve_n2_swap(const Graph &g, vertex v);

// --- Algorithme Complet ---
struct HCConfig {
    bool useN1 = true;
    bool useN2Add = true;
    bool useSwap = true;
    bool useDestruction = true;
    double destructRate = 0.5;
    int maxRestarts = 100;
};

std::vector<vertex> solve_hill_climber_complete(const Graph &g, const HCConfig &config);
