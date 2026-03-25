#include "hill_climber.hpp"
#include "utils.hpp"
#include <iostream>
#include <algorithm>
#include <ctime>

// --- Fonctions auxiliaires privées ---

/**
 * Calcul du degré résiduel plus performant (sans copie de vecteur).
 */
static gint compute_residual_degree(const Graph &g, const std::vector<vertex> &candidats, vertex v) {
    gint count = 0;
    for (vertex u : candidats) {
        if (u != v && g.is_edge(u, v)) {
            count++;
        }
    }
    return count;
}

// --- Implémentation des mouvements ---

bool move_n2_add(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats, bool best_improvement) {
    if (candidats.size() < 2) return false;

    vertex best_u = (vertex)-1, best_v = (vertex)-1;
    gint max_score = -1;

    for (size_t i = 0; i < candidats.size(); ++i) {
        for (size_t j = i + 1; j < candidats.size(); ++j) {
            vertex u = candidats[i];
            vertex v = candidats[j];

            if (g.is_edge(u, v)) {
                if (!best_improvement) {
                    // First Improvement : on s'arrête dès qu'on en trouve un
                    clique.push_back(u);
                    clique.push_back(v);
                    g.intersect_neighbors(candidats, u);
                    g.intersect_neighbors(candidats, v);
                    return true;
                }

                // Best Improvement : on cherche le score (somme degrés résiduels) max
                gint score = compute_residual_degree(g, candidats, u) + compute_residual_degree(g, candidats, v);
                if (score > max_score) {
                    max_score = score;
                    best_u = u;
                    best_v = v;
                }
            }
        }
    }

    if (best_u != (vertex)-1) {
        clique.push_back(best_u);
        clique.push_back(best_v);
        g.intersect_neighbors(candidats, best_u);
        g.intersect_neighbors(candidats, best_v);
        return true;
    }
    return false;
}

bool move_n1_add(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats) {
    if (candidats.empty()) return false;

    // On utilise l'heuristique max residual degree de utils.hpp
    vertex best_v = max_residual_degree_vertex(g, candidats).first;
    
    if (best_v != (vertex)-1) {
        clique.push_back(best_v);
        g.intersect_neighbors(candidats, best_v);
        return true;
    }
    return false;
}

bool move_n2_swap(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats) {
    if (clique.empty()) return false;

    // On parcourt chaque sommet w de la clique pour voir s'il peut être avantageusement remplacé
    for (size_t i = 0; i < clique.size(); ++i) {
        vertex w = clique[i];

        // 1. Détermine les candidats potentiels pour (Clique \ {w})
        // Ces candidats incluent les candidats actuels + ceux connectés à Clique\{w} mais PAS à w.
        std::vector<vertex> new_candidats;
        
        // On recalcule les candidats pour (Clique \ {w})
        // On commence par les voisins d'un autre sommet de la clique (si elle n'est pas vide)
        if (clique.size() > 1) {
            vertex other = (i == 0) ? clique[1] : clique[0];
            g.get_neighbors(other, new_candidats);
            for (size_t j = 0; j < clique.size(); ++j) {
                if (j != i && clique[j] != other) {
                    g.intersect_neighbors(new_candidats, clique[j]);
                }
            }
        } else {
            // Clique de taille 1, les candidats sont tous les sommets du graphe
            for (vertex v = 0; v < g.nb_vertices(); ++v) new_candidats.push_back(v);
        }

        // 2. Trouve s'il existe u dans new_candidats qui a un meilleur degré résiduel que w
        gint res_deg_w = compute_residual_degree(g, new_candidats, w);
        
        vertex best_u = (vertex)-1;
        gint max_res_deg_u = res_deg_w;

        for (vertex u : new_candidats) {
            if (u == w) continue;
            // On vérifie que u n'est pas déjà dans la clique
            bool already_in = false;
            for(vertex cv : clique) if(cv == u) { already_in = true; break; }
            if(already_in) continue;

            gint res_deg_u = compute_residual_degree(g, new_candidats, u);
            if (res_deg_u > max_res_deg_u) {
                max_res_deg_u = res_deg_u;
                best_u = u;
            }
        }

        if (best_u != (vertex)-1) {
            // Swap ! On retire w et on ajoute u
            clique.erase(clique.begin() + i);
            clique.push_back(best_u);
            
            // On met à jour les candidats officiels pour la nouvelle clique
            candidats = new_candidats;
            g.intersect_neighbors(candidats, best_u);
            return true;
        }
    }
    return false;
}

void perturb_destruction(const Graph &g, std::vector<vertex> &clique, std::vector<vertex> &candidats, double rate) {
    int to_remove = static_cast<int>(clique.size() * rate);
    if (to_remove == 0 && !clique.empty()) to_remove = 1;

    std::shuffle(clique.begin(), clique.end(), std::mt19937(std::random_device()()));
    
    for (int k = 0; k < to_remove && !clique.empty(); ++k) {
        clique.pop_back();
    }
    
    // Recalcule les candidats pour la nouvelle clique
    if (clique.empty()) {
        candidats.clear();
        for (vertex v = 0; v < g.nb_vertices(); ++v) candidats.push_back(v);
    } else {
        g.get_neighbors(clique[0], candidats);
        for (size_t i = 1; i < clique.size(); ++i) {
            g.intersect_neighbors(candidats, clique[i]);
        }
    }
}

std::vector<vertex> solve_hill_climber(const Graph &g, const HCConfig &config) {
    std::vector<vertex> best_clique;
    
    for (int restart = 0; restart < config.maxRestarts; ++restart) {
        std::vector<vertex> current_clique;
        std::vector<vertex> candidats;
        
        // Initialisation aléatoire ou premier sommet
        vertex start_v = rand() % g.nb_vertices();
        current_clique.push_back(start_v);
        g.get_neighbors(start_v, candidats);
        
        bool progressed = true;
        while (progressed) {
            progressed = false;
            
            // 1. Tente N2-Add
            if (move_n2_add(g, current_clique, candidats, config.useBestN2)) {
                progressed = true;
                continue;
            }
            
            // 2. Tente N1-Fallback
            if (config.useN1Fallback && move_n1_add(g, current_clique, candidats)) {
                progressed = true;
                continue;
            }
            
            // 3. Tente N2-Swap (Si bloqué)
            if (config.useSwap && move_n2_swap(g, current_clique, candidats)) {
                progressed = true;
                continue;
            }
        }
        
        if (current_clique.size() > best_clique.size()) {
            best_clique = current_clique;
        }
        
        // Optionnel : Destruction et boucle ou restart complet
    }
    
    return best_clique;
}
