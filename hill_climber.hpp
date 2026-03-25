#pragma once

#include "graph.hpp"
#include <vector>
#include <random>

/**
 * Structure de configuration pour l'algorithme Hill Climber.
 */
struct HCConfig {
    bool useBestN2 = true;      // true: Best Improvement, false: First Improvement
    bool useN1Fallback = true;
    bool useSwap = true;
    double destructRate = 0.5;
    int maxRestarts = 100;
};

/**
 * Tente d'ajouter deux sommets à la clique (Voisinage N2 - Croissance).
 * stratégie : Best Improvement (max somme degrés résiduels) ou First Improvement.
 */
bool move_n2_add(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats, bool best_improvement);

/**
 * Tente d'ajouter un seul sommet à la clique (Repli N1).
 */
bool move_n1_add(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats);

/**
 * Tente un échange 1-pour-1 (Voisinage N2 - Swap).
 * Retire w et ajoute u si res_deg(u) > res_deg(w).
 */
bool move_n2_swap(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats);

/**
 * Perturbation par destruction aléatoire d'une partie de la clique.
 */
void perturb_destruction(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats, double rate);

/**
 * Algorithme complet Hill Climber avec redémarrages (Multi-start).
 */
std::vector<vertex> solve_hill_climber(const Graph &g, const HCConfig &config);
