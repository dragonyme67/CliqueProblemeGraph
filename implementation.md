# Implémentation du Hill Climber pour le Maximum Clique Problem (Question 1)

Ce document décrit les algorithmes de recherche locale (Voisinage N1) déjà implémentés pour trouver une clique maximale.

## Voisinage N1 (Ajout d'un sommet)
Le voisinage N1 consiste à tenter d'agrandir la clique actuelle en y ajoutant un sommet choisi parmi les candidats (sommets connectés à tous les membres de la clique actuelle).

### 1. Best Improvement (Max Degré Résiduel)
*   **Logique** : Parmi tous les sommets candidats, on choisit celui qui possède le plus grand nombre de voisins au sein même de l'ensemble des candidats.
*   **Objectif** : Maximiser le nombre de candidats restants pour les étapes suivantes afin de construire une clique la plus grande possible.
*   **Code** : Implémenté via `MaxResidualDegreeStrategy` dans `select.cpp` et la fonction `residual_degree` dans `utils.cpp`.

### 2. Max Degree
*   **Logique** : On choisit le sommet candidat qui a le plus grand degré dans le graphe global.
*   **Objectif** : Utiliser une heuristique simple basée sur la connectivité globale du sommet.
*   **Code** : Fonction `descente_discret_best_improvement` dans `main.cpp`.

### 3. Random (Aléatoire)
*   **Logique** : On choisit un sommet au hasard parmi tous les sommets candidats compatibles.
*   **Objectif** : Explorer différentes zones de l'espace de recherche (utile pour des approches multi-start).
*   **Code** : Fonction `descente_discret_aleatoire` dans `main.cpp`.

### 4. First Improve (Premier croissant)
*   **Logique** : On parcourt les sommets dans l'ordre croissant de leurs indices et on ajoute le premier qui peut l'être (tout en restant une clique).
*   **Objectif** : Rapidité d'exécution sans calcul heuristique coûteux.
*   **Code** : Fonction `descente_discret_croissant` dans `main.cpp`.

### 5. Last Improve (Dernier décroissant)
*   **Logique** : On parcourt les sommets dans l'ordre décroissant de leurs indices et on ajoute le premier candidat trouvé.
*   **Objectif** : Similaire au First Improve, mais explore une autre partie de la liste.
*   **Code** : Fonction `descente_discret_decroissant` dans `main.cpp`.
