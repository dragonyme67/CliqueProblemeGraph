#include "clique_solver.hpp"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>

using namespace std;

void print_vector(vector<vertex> vec) {
  for (auto v : vec) {
    cout << " " << v;
  }
  cout << endl;
}

static void initialize_candidates(const Graph &g, const vector<vertex> &clique,
                                  vector<vertex> &candidates) {
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

vertex
MaxResidualDegreeStrategy::select(const Graph &g,
                                  const vector<vertex> &candidates) const {
  if (candidates.empty())
    return -1;
  return max_residual_degree_vertex(g, candidates).first;
}

vector<vertex> greedy_descent_n1(const Graph &g, vector<vertex> initial_clique,
                                 const Strategy &s) {
  vector<vertex> clique = initial_clique;
  vector<vertex> candidates;

  initialize_candidates(g, clique, candidates);

  while (!candidates.empty()) {
    vertex best_v = s.select(g, candidates);

    if (best_v == -1)
      break;

    auto it = find(candidates.begin(), candidates.end(), best_v);
    if (it != candidates.end()) {
      candidates.erase(it);
    } else {
      break;
    }

    clique.push_back(best_v);

    g.intersect_neighbors(candidates, best_v);
  }
  return clique;
}

vector<vertex> pair_descent_n2(const Graph &g, vector<vertex> initial_clique,
                               const Strategy &s, bool use_fallback) {
  vector<vertex> clique = initial_clique;
  vector<vertex> candidates;
  initialize_candidates(g, clique, candidates);

  while (candidates.size() >= 2) {
    vector<gint> res_degrees(candidates.size());
    for (size_t i = 0; i < candidates.size(); ++i) {
      res_degrees[i] = residual_degree(g, candidates, candidates[i]);
    }

    vertex best_u = -1, best_v = -1;
    gint max_score = -1;

    for (size_t i = 0; i < candidates.size(); ++i) {
      for (size_t j = i + 1; j < candidates.size(); ++j) {
        if (g.is_edge(candidates[i], candidates[j])) {
          gint score = res_degrees[i] + res_degrees[j];
          if (score > max_score) {
            max_score = score;
            best_u = candidates[i];
            best_v = candidates[j];
          }
        }
      }
    }

    if (best_u != -1) {
      clique.push_back(best_u);
      clique.push_back(best_v);
      g.intersect_neighbors(candidates, best_u);
      g.intersect_neighbors(candidates, best_v);
    } else {
      break;
    }
  }

  if (use_fallback) {
    return greedy_descent_n1(g, clique, s);
  }
  return clique;
}

vector<vertex> ruin_and_recreate(const Graph &g, vector<vertex> initial_clique,
                                 int iterations, int ruin_percent,
                                 const Strategy &s) {
  vector<vertex> current_clique = greedy_descent_n1(g, initial_clique, s);
  vector<vertex> best_clique = current_clique;

  default_random_engine engine(static_cast<unsigned int>(time(0)));

  for (int i = 0; i < iterations; ++i) {
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