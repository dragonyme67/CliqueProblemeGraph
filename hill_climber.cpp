#include "hill_climber.hpp"
#include "utils.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>

// --- Helper: Residual Degree plus rapide ---
static gint compute_residual_degree(const Graph &g,
                                    const std::vector<vertex> &candidats,
                                    vertex v) {
  gint count = 0;
  for (vertex u : candidats) {
    if (u != v && g.is_edge(u, v)) {
      count++;
    }
  }
  return count;
}

// --- Stratégie N1 (1 opération) : Best Improvement ---
std::vector<vertex> solve_n1_best(const Graph &g, vertex v) {
  std::vector<vertex> clique;
  clique.push_back(v);
  std::vector<vertex> candidats;
  g.get_neighbors(v, candidats);

  while (!candidats.empty()) {
    vertex best_v = -1;
    gint max_res_deg = -1;

    for (vertex c : candidats) {
      gint res_deg = compute_residual_degree(g, candidats, c);
      if (res_deg > max_res_deg) {
        max_res_deg = res_deg;
        best_v = c;
      }
    }

    if (best_v != -1) {
      clique.push_back(best_v);
      // On s'assure de l'intersecter ET de l'enlever de candidats
      auto it = std::find(candidats.begin(), candidats.end(), best_v);
      if (it != candidats.end())
        candidats.erase(it);
      g.intersect_neighbors(candidats, best_v);
    } else {
      break;
    }
  }
  return clique;
}

// --- Stratégie N2 (2 opérations) : Add Best improvement ---
std::vector<vertex> solve_n2_add_best(const Graph &g, vertex v) {
  std::vector<vertex> clique;
  clique.push_back(v);
  std::vector<vertex> candidats;
  g.get_neighbors(v, candidats);

  while (candidats.size() >= 2) {
    // Pré-calcul des degrés résiduels pour éviter le O(n^3)
    std::vector<gint> res_degrees(candidats.size());
    for (size_t i = 0; i < candidats.size(); ++i) {
      res_degrees[i] = compute_residual_degree(g, candidats, candidats[i]);
    }

    vertex best_u = -1, best_v = -1;
    gint max_score = -1;

    for (size_t i = 0; i < candidats.size(); ++i) {
      for (size_t j = i + 1; j < candidats.size(); ++j) {
        if (g.is_edge(candidats[i], candidats[j])) {
          gint score = res_degrees[i] + res_degrees[j];
          if (score > max_score) {
            max_score = score;
            best_u = candidats[i];
            best_v = candidats[j];
          }
        }
      }
    }

    if (best_u != -1) {
      clique.push_back(best_u);
      clique.push_back(best_v);
      g.intersect_neighbors(candidats, best_u);
      g.intersect_neighbors(candidats, best_v);
    } else {
      // Pas d'ajout N2 possible, on peut faire un repli N1 ou s'arrêter
      break;
    }
  }
  return clique;
}

// --- Stratégie N2 Swap ---
std::vector<vertex> solve_n2_swap(const Graph &g, vertex v) {
  std::vector<vertex> clique =
      solve_n1_best(g, v); // D'abord une bonne clique de base
  std::vector<vertex> candidats;
  // Initialiser candidats
  if (!clique.empty()) {
    g.get_neighbors(clique[0], candidats);
    for (size_t i = 1; i < clique.size(); ++i)
      g.intersect_neighbors(candidats, clique[i]);
  }

  // Un seul swap pour l'exemple (on pourrait boucler)
  for (size_t i = 0; i < clique.size(); ++i) {
    vertex w = clique[i];
    std::vector<vertex> new_candidats;
    // ... (Logique de swap 1-pour-1 simplifiée) ...
    // (Pour l'instant on se concentre sur N1 et N2 Add car le Swap est plus
    // complexe à implémenter par itéré)
  }
  return clique;
}

// --- Algorithme Complet ---
std::vector<vertex> solve_hill_climber_complete(const Graph &g,
                                                const HCConfig &config) {
  // ... (Logique complète par itération de N2, N1, Swap et Destruction)
  // On va juste faire un squelette pour l'instant pour prouver que N1 et N2
  // fonctionnent
  std::vector<vertex> best_clique;
  for (int i = 0; i < config.maxRestarts; ++i) {
    vertex start_v = rand() % g.nb_vertices();
    std::vector<vertex> c = solve_n1_best(g, start_v);
    if (c.size() > best_clique.size())
      best_clique = c;
  }
  return best_clique;
}
