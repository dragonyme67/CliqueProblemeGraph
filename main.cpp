#include "clique_solver.hpp"
#include "graphs/graphHeavy.hpp"
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

      {"C1000.9", 68},        {"C2000.9", 78},        {"DSJC1000_5", 15},

      {"DSJC500_5", 13},      {"C2000.5", 16},        {"C4000.5", 18},

      {"MANN_a27", 126},      {"MANN_a45", 344},      {"MANN_a81", 1100},

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

// Structure pour encapsuler les résultats
struct Result {
  string instance, algo, strategy;
  int nodes, edges, optimum, found, gap;
  long long time_ms;
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <instance1.clq> ..." << endl;
    return 1;
  }

  ofstream csv("benchmark_results.csv");
  csv << "Instance,GraphNodes,GraphEdges,Algo,Strategy,OptimumSize,CliqueFound,"
         "Time_ms,Gap\n";
  auto optimums = get_optimums();

  // On utilise des pointeurs bruts ici pour les stratégies car elles sont
  // stateless et partagées entre les threads.
  vector<unique_ptr<Strategy>> strategies;
  strategies.push_back(make_unique<FirstStrategy>());
  strategies.push_back(make_unique<LastStrategy>());
  strategies.push_back(make_unique<RandomStrategy>());
  strategies.push_back(make_unique<MaxResidualDegreeStrategy>());
for(int j = 0; j < 3; ++j){
  for (int i = 1; i < argc; ++i) {
    string filepath = argv[i];
    string instance_name = get_base_filename(filepath);

    cout << ">>> Loading: " << instance_name << endl;
    unique_ptr<GraphHeavy> g_ptr;
    try {
      g_ptr = make_unique<GraphHeavy>(filepath);
    } catch (...) {
      cerr << "Failed to load: " << filepath << endl;
      continue;
    }

    const Graph &g = *g_ptr;
    int opt_val = optimums.count(instance_name) ? optimums[instance_name] : -1;
    if(opt_val >= 0){
    // Liste des tâches asynchrones
    vector<future<void>> futures;

    auto run_task = [&](string algo_name, const Strategy *strat, auto func) {
      auto start = chrono::high_resolution_clock::now();
      vector<vertex> clique = func(g, vector<vertex>(), *strat);
      auto end = chrono::high_resolution_clock::now();

      auto time_ms =
          chrono::duration_cast<chrono::milliseconds>(end - start).count();
      int found_size = clique.size();

      if (found_size > 0 && !is_clique(g, clique))
        found_size = -2;
      int gap = (opt_val > 0 && found_size > 0) ? opt_val - found_size : -1;

      // Section critique pour l'écriture
      {
        lock_guard<mutex> lock_csv(csv_mutex);
        csv << instance_name << "," << g.nb_vertices() << "," << nb_edges(g)
            << "," << algo_name << "," << strat->toString() << "," << opt_val
            << "," << found_size << "," << time_ms << "," << gap << "\n";
        csv.flush();
      }
      {
        lock_guard<mutex> lock_cout(cout_mutex);
        cout << "  [DONE] " << algo_name << " (" << strat->toString() << ") -> "
             << found_size << " in " << time_ms << "ms" << endl;
      }
    };

    // Lancement des threads pour chaque combinaison
    for (const auto &strategy : strategies) {
      futures.push_back(async(
          launch::async, run_task, "N1_Greedy", strategy.get(),
          [](const Graph &g, const vector<vertex> &init, const Strategy &s) {
            return greedy_descent_n1(g, init, s);
          }));

      futures.push_back(async(
          launch::async, run_task, "N2_Pair_Fallback", strategy.get(),
          [](const Graph &g, const vector<vertex> &init, const Strategy &s) {
            return pair_descent_n2(g, init, s, true);
          }));

      futures.push_back(async(
          launch::async, run_task, "N3_Triple", strategy.get(),
          [](const Graph &g, const vector<vertex> &init, const Strategy &s) {
            return triple_descent_n3(g, init, s);
          }));

      futures.push_back(async(
          launch::async, run_task, "Ruin_Recreate", strategy.get(),
          [](const Graph &g, const vector<vertex> &init, const Strategy &s) {
            return ruin_and_recreate(g, init, 10, 50, s);
          }));
    }

    // Attente de la fin de tous les algos pour ce graphe avant de passer au
    // suivant (mémoire)
    for (auto &f : futures)
      f.wait();
    cout << "Finished instance: " << instance_name
         << "\n------------------------------------" << endl;
  }
}
}
  csv.close();
  return 0;
}
