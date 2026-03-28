#include "clique_solver.hpp"
#include "graphs/graphHeavy.hpp"
#include "weightedgraphDefs.hpp"
#include "utils.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future> // Pour le multi-threading léger
#include <iostream>
#include <memory>
#include <mutex> // Pour protéger les ressources partagées
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

// Mutex pour éviter que les threads n'écrivent en même temps dans le CSV ou la
// console
mutex cout_mutex;
mutex csv_mutex;

unordered_map<string, int> get_optimums() {
  return {
      {"C125.9", 34},         {"C250.9", 44},         {"C500.9", 57},

      {"C2000.9", 78},        {"DSJC1000_5", 15},

      {"DSJC500_5", 13},      {"C2000.5", 16},        {"C4000.5", 18},

      {"MANN_a27", 126},      {"MANN_a45", 344},

      {"brock200_2", 12},     {"brock200_4", 17},     {"brock400_2", 29},

      {"brock400_4", 33},     {"brock800_2", 24},     {"brock800_4", 26},

      {"gen200_p0.9_44", 44}, {"gen200_p0.9_55", 55}, {"gen400_p0.9_55", 55},

      {"gen400_p0.9_65", 65}, {"gen400_p0.9_75", 75}, {"hamming10-4", 40},

      {"hamming8-4", 16},     {"keller4", 11},        {"keller5", 27},

      {"keller6", 59},        {"p_hat300-1", 8},      {"p_hat300-2", 25},

      {"p_hat300-3", 36},     {"p_hat700-1", 11},     {"p_hat700-2", 44},

      {"p_hat700-3", 62},     {"p_hat1500-1", 12},    {"p_hat1500-2", 65},

      {"p_hat1500-3", 94}};
}

string get_base_filename(const string &path) {
  size_t pos = path.find_last_of("/\\");
  string base = (pos != string::npos) ? path.substr(pos + 1) : path;
  size_t ext_pos = base.find_last_of(".");
  if (ext_pos != string::npos)
    base = base.substr(0, ext_pos);
  return base;
}


int main(int argc, char *argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " <instance1.clq> ..." << endl;
    return 1;
  }

  ofstream csv("benchmark_results.csv");
  csv << "Instance,GraphNodes,GraphEdges,Algo,Strategy,OptimumSize,CliqueFound,TotalWeight,Time_ms,Gap\n";
  auto optimums = get_optimums();

  // Initialisation des stratégies
  vector<unique_ptr<Strategy>> strategies;
  strategies.push_back(make_unique<FirstStrategy>());
  strategies.push_back(make_unique<LastStrategy>());
  strategies.push_back(make_unique<RandomStrategy>());
  strategies.push_back(make_unique<MaxResidualDegreeStrategy>());
  strategies.push_back(make_unique<WeightedStrategy>());

  for (int j = 0; j < stoi(argv[1]; ++j) {
    for (int i = 2; i < argc; ++i) {
      string filepath = argv[i];
      string instance_name = get_base_filename(filepath);
      
      if (optimums.find(instance_name) == optimums.end()) {
        // Optionnel : décommenter pour voir ce qui est ignoré
        // cout << ">>> Skipping: " << instance_name << " (Not in benchmark list)" << endl;
        continue; 
      }

      unique_ptr<GraphHeavy> g_ptr;
      try {
        g_ptr = make_unique<GraphHeavy>(filepath);
      } catch (...) {
        cerr << "Failed to load: " << filepath << endl;
        continue;
      }


      const Graph &g = *g_ptr;
      int opt_val = optimums.count(instance_name) ? optimums[instance_name] : -1;
      
      cout << ">>> Instance: " << instance_name << " (Pass " << j+1 << ")" << endl;

      // Définition des algos à tester séquentiellement
      struct Algo {
        string name;
        function<vector<vertex>(const Graph&, const Strategy&)> run;
      };

      vector<Algo> test_suite = {
        {"N1_Greedy", [](const Graph& g, const Strategy& s) { return greedy_descent_n1(g, {}, s); }},
        {"N2_Pair_Fallback", [](const Graph& g, const Strategy& s) { return pair_descent_n2(g, {}, s, true); }},
        {"N2_Pair_NoFallback", [](const Graph& g, const Strategy& s) { return pair_descent_n2(g, {}, s, false); }},
        {"N3_Triple", [](const Graph& g, const Strategy& s) { return triple_descent_n3(g, {}, s); }},
        {"Ruin_Recreate", [](const Graph& g, const Strategy& s) { return ruin_and_recreate(g, {}, 10, 50, s); }}
      };

      // Pour chaque algorithme, on lance toutes les stratégies en parallèle
      for (const auto& algo : test_suite) {
        vector<future<void>> futures;

        for (const auto& strat_ptr : strategies) {
          const Strategy* s = strat_ptr.get();

          futures.push_back(async(launch::async, [&, s, algo, instance_name, opt_val]() {
            auto start = chrono::high_resolution_clock::now();
            
            // Exécution de l'algorithme avec la stratégie donnée
            vector<vertex> clique = algo.run(g, *s);
            
            auto end = chrono::high_resolution_clock::now();
            auto time_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

            // Calcul du poids et validation
            int found_size = clique.size();
            long long total_weight = 0;
            if (found_size > 0 && is_clique(g, clique)) {
              for (vertex v : clique) total_weight += getVertexWeight(g, v);
            } else if (found_size > 0) {
              found_size = -2; // Erreur de validité
            }

            int gap = (opt_val > 0 && found_size > 0) ? opt_val - found_size : -1;

            // Sorties sécurisées par mutex
            {
              lock_guard<mutex> lock_csv(csv_mutex);
              csv << instance_name << "," << g.nb_vertices() << "," << nb_edges(g)
                  << "," << algo.name << "," << s->toString() << "," << opt_val
                  << "," << found_size << "," << total_weight << "," << time_ms << "," << gap << "\n";
              csv.flush();
            }
            {
              lock_guard<mutex> lock_cout(cout_mutex);
              cout << "    [" << algo.name << "] " << s->toString() << " -> Size: " 
                   << found_size << " | Weight: " << total_weight << " (" << time_ms << "ms)" << endl;
            }
          }));
        }
        // Attente de la fin de la vague de stratégies pour cet algorithme
        for (auto &f : futures) f.wait();
      }
      cout << "------------------------------------" << endl;
    }
  }
  csv.close();
  return 0;
}
