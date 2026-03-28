#include "clique_solver.hpp"
#include "graph.hpp"
#include "utils.hpp" // residual_degree, initialize_candidates, greedy_descent_n1
#include <algorithm>
#include <chrono>
#include <ctime>
#include <random>
#include "weightedgraphDefs.cpp"

using namespace std;

static void initialize_candidates(const Graph &g, const vector<vertex> &clique,
                                  vector<vertex> &candidates) {
  candidates.clear();
  if (clique.empty()) {
    for (vertex i = 0; i < (vertex)g.nb_vertices(); ++i) {
      candidates.push_back(i);
    }
  } else {
    g.get_neighbors(clique[0], candidates);
    for (size_t i = 1; i < clique.size(); ++i) {
      g.intersect_neighbors(candidates, clique[i]);
    }
  }
}

vertex MaxResidualDegreeStrategy::select(const Graph &g, const vector<vertex> &candidates) const {
  if (candidates.empty())
    return -1;
  return max_residual_degree_vertex(g, candidates).first;
}

vertex WeightedStrategy::select(const Graph &g, const vector<vertex> &candidates) const {
    if (candidates.empty()) return -1;

    vertex best_v = -1;
    double best_score = -1.0;

    for (vertex v : candidates) {
        weight w = getVertexWeight(g, v);
        // On utilise le degré résiduel pour normaliser le poids
        // Plus le ratio est élevé, plus le sommet est "rentable"
        double score = (double)w / (1.0 + residual_degree(g, candidates, v));
        
        if (score > best_score) {
            best_score = score;
            best_v = v;
        }
    }
    return best_v;
}

// -------------------------------------------------------------------
// Helpers locaux (étapes du hill-climbing)
// Chaque fonction tente une amélioration et renvoie true si elle a réussi.
// Dans ce cas elle met à jour clique et candidates directement.
// -------------------------------------------------------------------

// Precompute residual degrees once per iteration
static vector<gint> compute_res_degrees(const Graph &g,
                                        const vector<vertex> &candidates) {
  vector<gint> res(candidates.size());
  for (size_t i = 0; i < candidates.size(); ++i)
    res[i] = residual_degree(g, candidates, candidates[i]);
  return res;
}

// Étape 1 : tente d'ajouter le meilleur triplet (+3)
static bool try_add_triple(const Graph &g, vector<vertex> &clique,
                           vector<vertex> &candidates,
                           const vector<gint> &res_degrees) {
  if (candidates.size() < 3)
    return false;

  vertex best_u = -1, best_v = -1, best_w = -1;
  gint max_score = -1;

  for (size_t i = 0; i < candidates.size(); ++i) {
    for (size_t j = i + 1; j < candidates.size(); ++j) {
      if (!g.is_edge(candidates[i], candidates[j]))
        continue;
      for (size_t k = j + 1; k < candidates.size(); ++k) {
        if (!g.is_edge(candidates[i], candidates[k]))
          continue;
        if (!g.is_edge(candidates[j], candidates[k]))
          continue;

        gint score = res_degrees[i] + res_degrees[j] + res_degrees[k];
        if (score > max_score) {
          max_score = score;
          best_u = candidates[i];
          best_v = candidates[j];
          best_w = candidates[k];
        }
      }
    }
  }

  if (best_u == -1)
    return false;

  clique.push_back(best_u);
  clique.push_back(best_v);
  clique.push_back(best_w);
  g.intersect_neighbors(candidates, best_u);
  g.intersect_neighbors(candidates, best_v);
  g.intersect_neighbors(candidates, best_w);
  return true;
}

// Étape 2 : tente d'ajouter la meilleure paire (+2)
static bool try_add_pair(const Graph &g, vector<vertex> &clique,
                         vector<vertex> &candidates,
                         const vector<gint> &res_degrees) {
  if (candidates.size() < 2)
    return false;

  vertex best_u = -1, best_v = -1;
  gint max_score = -1;

  for (size_t i = 0; i < candidates.size(); ++i) {
    for (size_t j = i + 1; j < candidates.size(); ++j) {
      if (!g.is_edge(candidates[i], candidates[j]))
        continue;

      gint score = res_degrees[i] + res_degrees[j];
      if (score > max_score) {
        max_score = score;
        best_u = candidates[i];
        best_v = candidates[j];
      }
    }
  }

  if (best_u == -1)
    return false;

  clique.push_back(best_u);
  clique.push_back(best_v);
  g.intersect_neighbors(candidates, best_u);
  g.intersect_neighbors(candidates, best_v);
  return true;
}

static bool try_add_weighted_pair(const Graph &g, vector<vertex> &clique,
                                  vector<vertex> &candidates) {
    if (candidates.size() < 2) return false;

    vertex best_u = -1, best_v = -1;
    weight max_added_weight = 0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (!g.is_edge(candidates[i], candidates[j])) continue;

            // On cherche à maximiser le gain de poids total
            weight current_pair_weight = getVertexWeight(g, candidates[i]) + 
                                         getVertexWeight(g, candidates[j]);
            
            if (current_pair_weight > max_added_weight) {
                max_added_weight = current_pair_weight;
                best_u = candidates[i];
                best_v = candidates[j];
            }
        }
    }

    if (best_u == -1) return false;

    clique.push_back(best_u);
    clique.push_back(best_v);
    g.intersect_neighbors(candidates, best_u);
    g.intersect_neighbors(candidates, best_v);
    return true;
}

// Étape 3 : tente d'ajouter un seul sommet (+1)
static bool try_add_one(const Graph &g, vector<vertex> &clique,
                        vector<vertex> &candidates, const Strategy &s) {
  vertex v = s.select(g, candidates);
  if (v == -1)
    return false;

  auto it = std::find(candidates.begin(), candidates.end(), v);
  if (it == candidates.end())
    return false;

  clique.push_back(v);
  g.intersect_neighbors(candidates, v);
  return true;
}

// Étape 4 : tente un échange 1-pour-2 (taille +1)
// On retire chaque sommet de la clique tour à tour, puis on délègue
// la recherche d'une paire à try_add_pair — sans dupliquer cette logique.
static bool try_swap_1_for_2(const Graph &g, vector<vertex> &clique,
                             vector<vertex> &candidates) {
  for (size_t i = 0; i < clique.size(); ++i) {
    // Clique réduite sans clique[i]
    vector<vertex> reduced_clique;
    reduced_clique.reserve(clique.size() - 1);
    for (size_t k = 0; k < clique.size(); ++k)
      if (k != i)
        reduced_clique.push_back(clique[k]);

    vector<vertex> new_candidates;
    initialize_candidates(g, reduced_clique, new_candidates);

    // On réutilise try_add_pair : si elle trouve une paire,
    // reduced_clique et new_candidates sont déjà mis à jour.
    vector<gint> res_degrees = compute_res_degrees(g, new_candidates);
    if (try_add_pair(g, reduced_clique, new_candidates, res_degrees)) {
      clique = reduced_clique;
      initialize_candidates(g, clique, candidates);
      return true;
    }
  }
  return false;
}

// -------------------------------------------------------------------
// Fonctions de descente utilisant les helpers
// -------------------------------------------------------------------

vector<vertex> greedy_descent_n1(const Graph &g, vector<vertex> initial_clique,
                                 const Strategy &s) {
  vector<vertex> clique = std::move(initial_clique);
  vector<vertex> candidates;
  initialize_candidates(g, clique, candidates);

  auto start_time = std::chrono::steady_clock::now();

  while (!candidates.empty()) {
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - start_time)
                       .count();
    if (elapsed >= 3600)
      return {};

    if (!try_add_one(g, clique, candidates, s))
      break;
  }
  return clique;
}

vector<vertex> pair_descent_n2(const Graph &g, vector<vertex> initial_clique,
                               const Strategy &s, bool use_fallback) {
  vector<vertex> clique = std::move(initial_clique);
  vector<vertex> candidates;
  initialize_candidates(g, clique, candidates);

  auto start_time = std::chrono::steady_clock::now();

  while (candidates.size() >= 2) {
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - start_time)
                       .count();
    if (elapsed >= 3600)
      return {};

    vector<gint> res_degrees = compute_res_degrees(g, candidates);
    if (!try_add_pair(g, clique, candidates, res_degrees))
      break;
  }

  if (use_fallback)
    return greedy_descent_n1(g, clique, s);

  return clique;
}

// -------------------------------------------------------------------
// Fonction principale
// -------------------------------------------------------------------
vector<vertex> triple_descent_n3(const Graph &g, vector<vertex> initial_clique,
                                 const Strategy &s) {
  vector<vertex> clique = std::move(initial_clique);
  vector<vertex> candidates;
  initialize_candidates(g, clique, candidates);

  auto start_time = std::chrono::steady_clock::now();

  bool improved = true;
  while (improved && !candidates.empty()) {

    // Timeout de 60 secondes
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - start_time)
                       .count();
    if (elapsed >= 3600)
      return {};

    // Degrés résiduels calculés une seule fois par itération
    vector<gint> res_degrees = compute_res_degrees(g, candidates);

    // Essai des étapes par ordre de priorité (meilleure amélioration d'abord)
    improved = try_add_triple(g, clique, candidates, res_degrees) ||
               try_add_pair(g, clique, candidates, res_degrees) ||
               try_add_one(g, clique, candidates, s) ||
               try_swap_1_for_2(g, clique, candidates);
  }

  return clique;
}

vector<vertex> ruin_and_recreate(const Graph &g, vector<vertex> initial_clique,
                                 int iterations, int ruin_percent,
                                 const Strategy &s) {
  vector<vertex> current_clique = greedy_descent_n1(g, initial_clique, s);
  vector<vertex> best_clique = current_clique;

  default_random_engine engine(static_cast<unsigned int>(time(0)));

  auto start_time = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i) {
    if (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time)
            .count() >= 3600)
      return vector<vertex>();
    if (current_clique.size() > 1) {
      shuffle(current_clique.begin(), current_clique.end(), engine);
      int keep_size =
          max(1, (int)current_clique.size() * (100 - ruin_percent) / 100);
      current_clique.resize(keep_size);
    }

    current_clique = greedy_descent_n1(g, current_clique, s);

    if (current_clique.size() > best_clique.size()) {
      best_clique = current_clique;
    }
  }
  return best_clique;
}
