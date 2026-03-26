#include "clique_solver.hpp"
#include "graph.hpp"
#include "utils.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>

// ═══════════════════════════════════════════════════════════════
// Utilitaire d'affichage (code existant de select.cpp)
// ═══════════════════════════════════════════════════════════════

void print_vector(vector<vertex> vec) {
  for (auto v : vec) {
    cout << " " << v;
  }
  cout << endl;
}

// ═══════════════════════════════════════════════════════════════
// Stratégie Max Residual Degree (code existant de select.cpp)
// ═══════════════════════════════════════════════════════════════

vertex
MaxResidualDegreeStrategy::select(const Graph &g,
                                  const std::vector<vertex> &restants) const {
  if (restants.empty())
    return -1;
  return max_residual_degree_vertex(g, restants).first;
}

// ═══════════════════════════════════════════════════════════════
// N1 Récursif (code existant de select.cpp)
// ═══════════════════════════════════════════════════════════════

bool descente_n1(const Graph &g, std::vector<vertex> &clique,
                 std::vector<vertex> restants, const Strategy &s) {
  vertex choisi = s.select(g, restants);

  if (choisi == -1) {
    cout << s.toString() << " : ";
    print_vector(clique);
    return false;
  }

  clique.push_back(choisi);

  // Intersection restants, choisi
  std::vector<vertex> nouveaux_restants;
  for (vertex v : restants) {
    if (v != choisi && g.is_edge(v, choisi)) {
      nouveaux_restants.push_back(v);
    }
  }

  restants = nouveaux_restants;

  if (!descente_n1(g, clique, restants, s)) {
    return true;
  }

  return true;
}

// ═══════════════════════════════════════════════════════════════
// Helper : Residual Degree (code existant de hill_climber.cpp)
// ═══════════════════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════════════════
// N1 Itératif (code existant de hill_climber.cpp / solve_n1_best)
// Modifié pour accepter une Strategy au lieu du degré résiduel
// codé en dur
// ═══════════════════════════════════════════════════════════════

std::vector<vertex> descente_n1_iterative(const Graph &g, vertex v,
                                          const Strategy &s) {
  std::vector<vertex> clique;
  clique.push_back(v);
  std::vector<vertex> candidats;
  g.get_neighbors(v, candidats);

  while (!candidats.empty()) {
    vertex best_v = s.select(g, candidats);

    if (best_v != -1) {
      clique.push_back(best_v);
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

// ═══════════════════════════════════════════════════════════════
// N2-Add (code existant de hill_climber.cpp / solve_n2_add_best)
// ═══════════════════════════════════════════════════════════════

std::vector<vertex> descente_n2_add(const Graph &g, vertex v) {
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
      break;
    }
  }
  return clique;
}

// ═══════════════════════════════════════════════════════════════
// Multi-start (NOUVEAU — ~10 lignes)
// ═══════════════════════════════════════════════════════════════

std::vector<vertex> multi_start(const Graph &g, int nb_restarts,
                                DescentFunction descente) {
  std::vector<vertex> best_clique;
  for (int i = 0; i < nb_restarts; ++i) {
    vertex start_v = rand() % g.nb_vertices();
    std::vector<vertex> c = descente(g, start_v);
    if (c.size() > best_clique.size())
      best_clique = c;
  }
  return best_clique;
}
