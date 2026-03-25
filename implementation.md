# Implémentation du Hill Climber N2 (Détails Techniques)

Ce document explique les choix d'implémentation effectués pour l'algorithme Hill Climber avec voisinage N2.

## 1. Principes de base et Optimisation
Conformément aux consignes, l'implémentation repose sur la gestion d'un ensemble de **candidats** (sommets connectés à l'intégralité de la clique actuelle).

*   **Utilisation des Intersections** : Au lieu d'utiliser `is_clique()` (coûteux en $O(k^2)$), nous maintenons le vecteur de candidats via la fonction `g.intersect_neighbors()`. À chaque ajout de sommet, la liste des candidats est réduite à l'intersection des voisins du nouveau sommet.
*   **Focus sur le Graphe Résiduel** : Toutes les fonctions de sélection (N1, N2) travaillent sur le **graphe induit par les candidats** (graphe résiduel) et non sur les degrés globaux du graphe de départ. Cela permet de prendre des décisions basées sur le potentiel réel d'agrandissement de la clique.

## 2. Voisinage N2 (2 actions)

### A. Croissance (Add + Add)
Cette fonction cherche une paire de sommets `(u, v)` dans l'ensemble des candidats qui sont connectés entre eux.
*   **Best Improvement** : Calcule le score de chaque paire comme la somme de leurs **degrés résiduels** (nombre de voisins au sein du même ensemble de candidats). On choisit la paire maximisant ce score.
*   **First Improvement** : Retourne la première paire connectée trouvée pour accélérer le processus sur les graphes denses.

### B. Swap (Remove + Add)
Si aucun ajout n'est possible, on tente d'échanger un sommet `w` de la clique par un sommet `u` extérieur.
*   **Logique** : Le sommet `u` doit être connecté à `Clique \ {w}`.
*   **Critère** : On effectue le swap uniquement si le degré résiduel de `u` (potentiel futur) est strictement supérieur à celui de `w`. Cela permet de "débloquer" une situation figée.

## 3. Stratégies Méta-heuristiques

### Iterated Local Search (ILS) et Destruction
L'algorithme utilise maintenant un booléen `useDestruction` dans la configuration :
*   **Destruction** : Lorsqu'un optimum local est atteint, au lieu de tout supprimer, on retire aléatoirement 50% des sommets (taux paramétrable).
*   **Redémarrage (Multi-start)** : Le processus est répété `maxRestarts` fois. On conserve toujours la meilleure clique trouvée depuis le début de l'exécution.

## 4. Organisation du code
Le code est séparé dans les fichiers suivants pour une meilleure clarté :
*   `hill_climber.hpp` : Structures de données et signatures.
*   `hill_climber.cpp` : Logique algorithmique utilisant les fonctions de `utils.cpp` et les méthodes d'intersection de la classe `Graph`.
