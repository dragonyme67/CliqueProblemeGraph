#include "clique_solver.hpp"
#include "graphs/graphHeavy.hpp"

#include "utils.hpp"
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

void test_algorithm(const string &algo_name, const Graph &g,
                    const vector<vertex> &clique) {
  cout << "  " << algo_name << " clique size: " << clique.size();
  if (is_clique(g, clique)) {
    cout << " (Validated)";
  } else {
    cout << " (INVALID CLIQUE!)";
  }
  cout << endl;
}

int main(int argc, char *argv[]) {
  string s("C125.9.clq");
  if (argc >= 2) {
    s = argv[1];
  }

  // Try to load the graph, default to a random graph if it fails or if no file
  // is provided For now, let's use the random graph as in the original main.cpp
  GraphHeavy g(s);

  cout << "Graph Information:" << endl;
  cout << "  Vertices: " << g.nb_vertices() << endl;
  cout << "  Edges:    " << nb_edges(g) << endl;
  cout << "  Density:  " << density(g) << endl;
  cout << "  Max Deg:  " << max_degree(g) << endl;
  cout << "------------------------------------" << endl;

  vector<unique_ptr<Strategy>> strategies;
  strategies.push_back(make_unique<FirstStrategy>());
  strategies.push_back(make_unique<LastStrategy>());
  strategies.push_back(make_unique<RandomStrategy>());
  strategies.push_back(make_unique<MaxResidualDegreeStrategy>());

  for (const auto &strategy : strategies) {
    cout << "Strategy: " << strategy->toString() << endl;

    // Test N1 Greedy Descent
    vector<vertex> initial_clique;
    vector<vertex> clique_n1 = greedy_descent_n1(g, initial_clique, *strategy);
    test_algorithm("N1 Greedy", g, clique_n1);

    bool use_fallback = true;
    vector<vertex> clique_n2_with_fallback =
        pair_descent_n2(g, initial_clique, *strategy, use_fallback);
    test_algorithm("N2 Pair with FallBack ", g, clique_n2_with_fallback);

    use_fallback = false;
    vector<vertex> clique_n2_no_fallback =
        pair_descent_n2(g, initial_clique, *strategy, use_fallback);
    test_algorithm("N2 Pair without FallBack ", g, clique_n2_no_fallback);

    // Test Ruin and Recreate

    int iterations = 10;
    int ruin_percent = 50;
    vector<vertex> clique_rr = ruin_and_recreate(g, initial_clique, iterations,
                                                 ruin_percent, *strategy);
    test_algorithm("R&R (100)", g, clique_rr);

    cout << "------------------------------------" << endl;
  }

  return 0;
}
