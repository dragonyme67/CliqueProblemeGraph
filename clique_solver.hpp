#pragma once

#include "graph.hpp"
#include "utils.hpp"
#include <functional>
#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════
// 1. STRATÉGIES DE SÉLECTION (pattern Strategy)
// ═══════════════════════════════════════════════════════════════

class Strategy {
public:
  virtual ~Strategy() {}
  virtual vertex select(const Graph &g,
                        const std::vector<vertex> &restants) const = 0;
  virtual string toString() const = 0;
};

// --- Stratégies existantes ---

class MaxResidualDegreeStrategy : public Strategy {
public:
  ~MaxResidualDegreeStrategy() = default;
  vertex select(const Graph &g,
                const std::vector<vertex> &restants) const override;
  string toString() const override { return "Max residual degree"; };
};

// --- Nouvelles stratégies ---

class FirstStrategy : public Strategy {
public:
  vertex select(const Graph &g,
                const std::vector<vertex> &restants) const override {
    return restants.empty() ? -1 : restants.front();
  }
  string toString() const override { return "First"; }
};

class LastStrategy : public Strategy {
public:
  vertex select(const Graph &g,
                const std::vector<vertex> &restants) const override {
    return restants.empty() ? -1 : restants.back();
  }
  string toString() const override { return "Last"; }
};

class RandomStrategy : public Strategy {
public:
  vertex select(const Graph &g,
                const std::vector<vertex> &restants) const override {
    return restants.empty() ? -1
                            : restants[rand() % restants.size()];
  }
  string toString() const override { return "Random"; }
};

class MaxDegreeStrategy : public Strategy {
public:
  vertex select(const Graph &g,
                const std::vector<vertex> &restants) const override {
    if (restants.empty())
      return -1;
    vertex best = restants[0];
    gint maxD = g.degree(restants[0]);
    for (vertex v : restants) {
      if (g.degree(v) > maxD) {
        maxD = g.degree(v);
        best = v;
      }
    }
    return best;
  }
  string toString() const override { return "Max degree"; }
};

// ═══════════════════════════════════════════════════════════════
// 2. FONCTIONS DE DESCENTE
// ═══════════════════════════════════════════════════════════════

void print_vector(vector<vertex> vec);

// N1 récursif — utilise une Strategy pour choisir le prochain sommet
// (code existant de select.cpp)
bool descente_n1(const Graph &g, std::vector<vertex> &clique,
                 std::vector<vertex> restants, const Strategy &s);

// N1 itératif — utilise intersect_neighbors (plus efficace)
// (code existant de hill_climber.cpp / solve_n1_best)
std::vector<vertex> descente_n1_iterative(const Graph &g, vertex v,
                                          const Strategy &s);

// N2-Add — cherche la meilleure paire connectée
// (code existant de hill_climber.cpp / solve_n2_add_best)
std::vector<vertex> descente_n2_add(const Graph &g, vertex v);

// ═══════════════════════════════════════════════════════════════
// 3. MÉTA-HEURISTIQUE
// ═══════════════════════════════════════════════════════════════

using DescentFunction =
    std::function<std::vector<vertex>(const Graph &, vertex)>;

// Multi-start : relance N fois depuis un sommet aléatoire, garde la meilleure
std::vector<vertex> multi_start(const Graph &g, int nb_restarts,
                                DescentFunction descente);
