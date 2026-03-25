
#include "graphs/graphAdjMatrix.hpp"
#include "graphs/graphListAdj.hpp"
#include "hill_climber.hpp"
#include "select.hpp"
#include "utils.hpp"

#include "utils.hpp"
#include <iostream>

void descente_discret_croissant(const Graph *g, vertex v) {
  vector<vertex> clique;
  clique.push_back(v);
  vector<vertex> neighbors;
  g->get_neighbors(clique.front(), neighbors);
  while (neighbors.size() > 0) {
    for (auto it = neighbors.begin(); it < neighbors.end(); it++) {
      clique.push_back(*it);
      if (is_clique(*g, clique)) {
        g->get_neighbors(clique.back(), neighbors);
        break;
      } else {
        clique.pop_back();
        neighbors.erase(it);
      }
    }
  }
  cout << "Clique croissante : ";
  print_vector(clique);
}

void descente_discret_decroissant(const Graph *g, vertex v) {
  vector<vertex> clique;
  clique.push_back(v);
  vector<vertex> neighbors;
  g->get_neighbors(clique.front(), neighbors);
  while (neighbors.size() > 0) {
    for (vertex i = neighbors.size() - 1; i >= 0; --i) {
      clique.push_back(neighbors.at(i));
      if (is_clique(*g, clique)) {
        g->get_neighbors(clique.back(), neighbors);
        break;
      } else {
        clique.pop_back();
        neighbors.erase(neighbors.begin() + i);
      }
    }
  }
  cout << "Clique Décroissante : ";
  print_vector(clique);
}

void descente_discret_aleatoire(const Graph *g, const vertex &v) {
  vector<vertex> clique;
  clique.push_back(v);
  vector<vertex> neighbors;
  vector<vertex> candidates;
  g->get_neighbors(clique.front(), neighbors);
  while (neighbors.size() > 0) {
    for (auto it = neighbors.begin(); it < neighbors.end(); it++) {
      clique.push_back(*it);
      if (is_clique(*g, clique))
        candidates.push_back(*it);
      else
        neighbors.erase(it);
      clique.pop_back();
    }
    if (candidates.size() > 0) {
      vertex candidat = candidates.at(rand() % candidates.size());
      candidates.clear();
      clique.push_back(candidat);
      g->get_neighbors(candidat, neighbors);
    }
  }
  cout << "Clique aléatoire : ";
  print_vector(clique);
}

void descente_discret_best_improvement(const Graph *g, vertex v) {
  vector<vertex> cliques;
  vector<vertex> neighbors;

  cliques.push_back(v);
  g->get_neighbors(cliques.front(), neighbors);

  while (!neighbors.empty()) {
    vertex best_v = -1;
    gint degre_best_v = -1;

    for (auto v : neighbors) {
      gint degre_v = g->degree(v);
      cliques.push_back(v);
      if (is_clique(*g, cliques)) {
        if (degre_v > degre_best_v) {
          best_v = v;
          degre_best_v = degre_v;
        }
      }
      cliques.pop_back();
    }
    if (best_v >= 0) {
      cliques.push_back(best_v);
      g->get_neighbors(best_v, neighbors);
    } else
      neighbors.clear();
  }
  cout << "Clique best improve : ";
  print_vector(cliques);
}

int main(int argc, char *argv[]) {
  // string s("myciel");
  string s("brock200_1.clq"); // nom de l'instance

  if (argc >= 2) {
    s = argv[1];
    // s="../instances/"+s;
  }
  // GraphAdjMatrix g(s);
  GraphHeavy g(s);
  // GraphAdjVectorSorted g(s);
  // GraphAdjVector g(s);
  // GraphAdjVector g(100,0.8,2);
  // GraphAdjMatrix g(100,0.8,2);
  // GraphHeavy g(100,0.8,2);

  // Returns the density of the graph.
  cout << "Nombre de sommets du graphe : " << g.nb_vertices() << endl;
  cout << "Nombre d arretes du graphe : " << nb_edges(g) << endl;
  cout << "Densite du graphe : " << density(g) << endl;
  cout << "Degre max du graphe : " << max_degree(g) << endl;
  cout << "Parcours BFS du graphe partant de 0 : ";
  vector<vertex> vertices_visit;
  breadth_first_search(g, 0, vertices_visit);
  print_vector(vertices_visit);
  vector<vector<vertex>> comp;
  connected_components(g, comp);
  for (auto a : comp) {
    cout << "Composante connexe : ";
    print_vector(a);
    cout << endl;
  }
  vector<vertex> subset_vertices{0, 1, 2, 15};
  print_vector(subset_vertices);
  connected_components(g, subset_vertices, comp);
  for (auto a : comp) {
    cout << "Composante connexe : ";
    print_vector(a);
    // cout<<endl;
  }

  cout << endl;
  cout << "       EXERCICE 1" << endl;
  srand((unsigned)time(0));
  vertex vRandom = (rand() % g.nb_vertices());
  std::vector<vertex> clique;
  std::vector<vertex> restant;
  g.get_neighbors(vRandom, restant);
  MaxResidualDegreeStrategy maxDegree;
  clique.push_back(vRandom);
  cout << "Sommet choisi : " << vRandom << endl;
  descente_n1(g, clique, restant, maxDegree);
  descente_discret_croissant(&g, vRandom);
  descente_discret_decroissant(&g, vRandom);
  descente_discret_aleatoire(&g, vRandom);
  descente_discret_best_improvement(&g, vRandom);

  cout << "       EXERCICE 1 - Tests indépendants" << endl;
  
  HCConfig config;
  config.maxRestarts = 10;
  
  vertex vRestart = rand() % g.nb_vertices();
  cout << "Sommet de départ : " << vRestart << endl;

  // Test 1: N1 Uniquement (Graphe résiduel)
  cout << "--- Test 1: N1 Best (Residual Degree) ---" << endl;
  vector<vertex> res_n1 = solve_n1_best(g, vRestart);
  cout << "Taille N1: " << res_n1.size() << " | Clique: "; print_vector(res_n1);

  // Test 2: N2 add Uniquement (Graphe résiduel)
  cout << "\n--- Test 2: N2-Add Best ---" << endl;
  vector<vertex> res_n2 = solve_n2_add_best(g, vRestart);
  cout << "Taille N2: " << res_n2.size() << " | Clique: "; print_vector(res_n2);

  // Test 3: Hill Climber Complet (10 restarts)
  cout << "\n--- Test 3: HC Complet (10 iterations) ---" << endl;
  vector<vertex> best = solve_hill_climber_complete(g, config);
  cout << "Meilleure taille trouvée: " << best.size() << endl;

  return 0;
}
