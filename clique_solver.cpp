#include "clique_solver.hpp"
#include "graph.hpp"
#include "utils.hpp" // residual_degree, initialize_candidates, greedy_descent_n1
#include "weightedgraphDefs.hpp"
#include "graphIO.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <random>

using namespace std;

static void initialize_candidates(const Graph &g, const vector<vertex> &clique,
                                  vector<vertex> &candidates)
{
  candidates.clear();
  if (clique.empty())
  {
    for (vertex i = 0; i < (vertex)g.nb_vertices(); ++i)
    {
      candidates.push_back(i);
    }
  }
  else
  {
    g.get_neighbors(clique[0], candidates);
    for (size_t i = 1; i < clique.size(); ++i)
    {
      g.intersect_neighbors(candidates, clique[i]);
    }
  }
}

vertex MaxResidualDegreeStrategy::select(const Graph &g, const vector<vertex> &candidates) const
{
  if (candidates.empty())
    return -1;
  return max_residual_degree_vertex(g, candidates).first;
}

vertex WeightedStrategy::select(const Graph &g, const vector<vertex> &candidates) const
{
  if (candidates.empty())
    return -1;

  vertex best_v = -1;
  double best_score = -1.0;

  for (vertex v : candidates)
  {
    weight w = getVertexWeight(g, v);
    // On utilise le degré résiduel pour normaliser le poids
    // Plus le ratio est élevé, plus le sommet est "rentable"
    double score = (double)w / (1.0 + residual_degree(g, candidates, v));

    if (score > best_score)
    {
      best_score = score;
      best_v = v;
    }
  }
  return best_v;
}

std::vector<gint> LastStrategy::get_ordered_indices(gint n) const
{
  std::vector<gint> indices(n);
  for (gint i = 0; i < n; ++i)
  {
    indices[i] = (n - 1) - i; // On commence par la fin : n-1, n-2...
  }
  return indices;
}

std::vector<gint> RandomStrategy::get_ordered_indices(gint n) const
{
  std::vector<gint> indices(n);
  for (gint i = 0; i < n; ++i)
    indices[i] = i;

  // Algorithme de Fisher-Yates simple avec rand()
  for (gint i = n - 1; i > 0; --i)
  {
    gint j = rand() % (i + 1);
    std::swap(indices[i], indices[j]);
  }
  return indices;
}

// -------------------------------------------------------------------
// Helpers locaux (étapes du hill-climbing)
// Chaque fonction tente une amélioration et renvoie true si elle a réussi.
// Dans ce cas elle met à jour clique et candidates directement.
// -------------------------------------------------------------------

// Precompute residual degrees once per iteration
static vector<gint> compute_res_degrees(const Graph &g,
                                        const vector<vertex> &candidates)
{
  vector<gint> res(candidates.size());
  for (size_t i = 0; i < candidates.size(); ++i)
    res[i] = residual_degree(g, candidates, candidates[i]);
  return res;
}

// Étape 1 : tente d'ajouter le meilleur triplet (+3)
static bool try_add_triple(const Graph &g, vector<vertex> &clique,
                           vector<vertex> &candidates, const Strategy &s) {
    gint n = (gint)candidates.size();
    if (n < 3) return false;

    vector<gint> indices = s.get_ordered_indices(n);
    bool best_mode = s.is_best_improvement();
    
    vertex b_u = -1, b_v = -1, b_w = -1;
    double max_s = -1.0;

    for (gint i : indices) {
        for (gint j : indices) {
            if (i >= j || !g.is_edge(candidates[i], candidates[j])) continue;
            for (gint k : indices) {
                if (j >= k) continue;
                
                vertex u = candidates[i], v = candidates[j], w = candidates[k];
                if (g.is_edge(u, w) && g.is_edge(v, w)) {
                    if (!best_mode) {
                        clique.insert(clique.end(), {u, v, w});
                        g.intersect_neighbors(candidates, u);
                        g.intersect_neighbors(candidates, v);
                        g.intersect_neighbors(candidates, w);
                        return true;
                    }
                    // Calcul du score (Poids ou Degré Résiduel)
                    double score = (s.toString() == "WeightedResidual") ? 
                        (double)(getVertexWeight(g, u) + getVertexWeight(g, v) + getVertexWeight(g, w)) :
                        (double)(residual_degree(g, candidates, u) + residual_degree(g, candidates, v) + residual_degree(g, candidates, w));
                    
                    if (score > max_s) { max_s = score; b_u = u; b_v = v; b_w = w; }
                }
            }
        }
    }
    if (best_mode && b_u != -1) {
        clique.insert(clique.end(), {b_u, b_v, b_w});
        g.intersect_neighbors(candidates, b_u); g.intersect_neighbors(candidates, b_v); g.intersect_neighbors(candidates, b_w);
        return true;
    }
    return false;
}

// Étape 2 : tente d'ajouter la meilleure paire (+2)
static bool try_add_pair(const Graph &g, vector<vertex> &clique,
                         vector<vertex> &candidates, const Strategy &s)
{
  gint n = (gint)candidates.size();
  if (n < 2)
    return false;

  // On récupère l'ordre dicté par la stratégie (ex: inverse pour Last, shuffle pour Random)
  std::vector<gint> indices = s.get_ordered_indices(n);

  bool best_mode = s.is_best_improvement();
  bool weighted_mode = (s.toString() == "WeightedResidual");

  vertex best_u = -1, best_v = -1;
  double max_score = -1.0;

  for (gint i_idx : indices)
  {
    for (gint j_idx : indices)
    {
      if (i_idx >= j_idx)
        continue;

      vertex u = candidates[i_idx];
      vertex v = candidates[j_idx];

      if (g.is_edge(u, v))
      {
        if (!best_mode)
        {
          // --- LOGIQUE FIRST IMPROVEMENT ---
          // On a trouvé une paire, on l'ajoute immédiatement
          clique.push_back(u);
          clique.push_back(v);
          g.intersect_neighbors(candidates, u);
          g.intersect_neighbors(candidates, v);
          return true;
        }
        else
        {
          // --- LOGIQUE BEST IMPROVEMENT ---
          // On calcule le score pour comparer avec le meilleur trouvé jusqu'ici
          double current_score;
          if (weighted_mode)
          {
            current_score = (double)(getVertexWeight(g, u) + getVertexWeight(g, v));
          }
          else
          {
            current_score = (double)(residual_degree(g, candidates, u) +
                                     residual_degree(g, candidates, v));
          }

          if (current_score > max_score)
          {
            max_score = current_score;
            best_u = u;
            best_v = v;
          }
        }
      }
    }
  }

  if (best_mode && best_u != -1)
  {
    clique.push_back(best_u);
    clique.push_back(best_v);
    g.intersect_neighbors(candidates, best_u);
    g.intersect_neighbors(candidates, best_v);
    return true;
  }

  return false;
}


// Étape 3 : tente d'ajouter un seul sommet (+1)
static bool try_add_one(const Graph &g, vector<vertex> &clique,
                        vector<vertex> &candidates, const Strategy &s)
{
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

// Échange 1 pour 1 : utile pour Max Weight
static bool try_swap_1_for_1(const Graph &g, vector<vertex> &clique, vector<vertex> &candidates, const Strategy &s) {
    bool weighted = (s.toString() == "WeightedResidual");
    
    for (size_t i = 0; i < clique.size(); ++i) {
        vertex old_v = clique[i];
        // On crée une clique temporaire sans le sommet i
        vector<vertex> sub_clique;
        for (size_t k = 0; k < clique.size(); ++k) if (k != i) sub_clique.push_back(clique[k]);

        vector<vertex> replacers;
        initialize_candidates(g, sub_clique, replacers);

        for (vertex new_v : replacers) {
            if (new_v == old_v) continue;
            
            // Condition d'amélioration
            bool improves = weighted ? (getVertexWeight(g, new_v) > getVertexWeight(g, old_v)) 
                                     : (residual_degree(g, replacers, new_v) > residual_degree(g, clique, old_v));
            
            if (improves) {
                clique = std::move(sub_clique);
                clique.push_back(new_v);
                initialize_candidates(g, clique, candidates);
                return true;
            }
        }
    }
    return false;
}

// Échange 1 pour 2 : augmente la taille de la clique
static bool try_swap_1_for_2(const Graph &g, vector<vertex> &clique, vector<vertex> &candidates, const Strategy &s) {
    for (size_t i = 0; i < clique.size(); ++i) {
        vector<vertex> sub_clique;
        for (size_t k = 0; k < clique.size(); ++k) if (k != i) sub_clique.push_back(clique[k]);

        vector<vertex> replacers;
        initialize_candidates(g, sub_clique, replacers);
        
        // On cherche s'il existe une PAIRE dans les replacers
        // On peut réutiliser notre logique de try_add_pair sur ces nouveaux candidats
        if (try_add_pair(g, sub_clique, replacers, s)) {
            clique = std::move(sub_clique);
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
                                 const Strategy &s)
{
  vector<vertex> clique = std::move(initial_clique);
  vector<vertex> candidates;
  initialize_candidates(g, clique, candidates);

  auto start_time = std::chrono::steady_clock::now();

  while (!candidates.empty())
  {
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - start_time)
                       .count();
    if (elapsed >= 600)
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
  bool improved = true;

  while (improved) {
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - start_time).count();
    if (elapsed >= 600) break;

    if (candidates.size() >= 2 && try_add_pair(g, clique, candidates, s)) {
        improved = true;
        continue;
    }
    if (try_swap_1_for_1(g, clique, candidates, s)) {
        improved = true;
        continue;
    }

    improved = false;
  }

  if (use_fallback) {
      return greedy_descent_n1(g, clique, s);
  }

  return clique;
}

// -------------------------------------------------------------------
// Fonction principale
// -------------------------------------------------------------------
vector<vertex> triple_descent_n3(const Graph &g, vector<vertex> initial_clique, const Strategy &s) {
    vector<vertex> clique = std::move(initial_clique);
    vector<vertex> candidates;
    initialize_candidates(g, clique, candidates);

    auto start_time = std::chrono::steady_clock::now();
    bool improved = true;

    while (improved) {
        // Vérification du chrono
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::steady_clock::now() - start_time).count();
        if (elapsed >= 600) break;

        // Ordre de recherche locale (du plus efficace au plus coûteux)
        improved = try_add_triple(g, clique, candidates, s) || // +3 sommets
                   try_add_pair(g, clique, candidates, s)   || // +2 sommets / 1 - 1 sommets
                   try_add_one(g, clique, candidates, s)    || // +1 sommet
                   try_swap_1_for_2(g, clique, candidates, s); // Swap qui gagne +1 en taille
    }
    return clique;
}

vector<vertex> ruin_and_recreate(const Graph &g, vector<vertex> initial_clique,
                                 int iterations, int ruin_percent,
                                 const Strategy &s)
{
  vector<vertex> current_clique = greedy_descent_n1(g, initial_clique, s);
  vector<vertex> best_clique = current_clique;

  default_random_engine engine(static_cast<unsigned int>(time(0)));

  auto start_time = std::chrono::steady_clock::now();
  for (int i = 0; i < iterations; ++i)
  {
    if (std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time)
            .count() >= 600)
      return vector<vertex>();
    if (current_clique.size() > 1)
    {
      shuffle(current_clique.begin(), current_clique.end(), engine);
      int keep_size =
          max(1, (int)current_clique.size() * (100 - ruin_percent) / 100);
      current_clique.resize(keep_size);
    }

    current_clique = greedy_descent_n1(g, current_clique, s);

    if (current_clique.size() > best_clique.size())
    {
      best_clique = current_clique;
    }
  }
  return best_clique;
}
