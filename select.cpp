#include "select.hpp"
#include "utils.hpp" // Pour max_residual_degree_vertex
#include "graph.hpp"
#include <iostream>


void print_vector(vector<vertex> vec){
    for (auto v : vec) {
        cout << " " << v;
    }
    cout << endl;
}


bool descente_n1(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> restants, const Strategy &s) {
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

    restants = nouveaux_resta

    if (!descente_n1(g, clique, restants, s)) {
        return true;
    }

    return true;
}



vertex MaxResidualDegreeStrategy::select(const Graph &g, const std::vector<vertex> &restants) const {
    if (restants.empty()) return -1;
    return max_residual_degree_vertex(g, restants).first;
}


/*
vertex MaxWeightStrategy::select(const Graph &g, const std::vector<vertex> &restants) const {
    vertex best_v = (vertex)-1;
    gint max_w = 0;
    for (vertex v : restants) {
        gint w = getVertexWeight(g, v); // [cite: 44]
        if (w > max_w) {
            max_w = w;
            best_v = v;
        }
    }
    return best_v;
}
*/
