# Algorithmes de recherche locale (Hill Climbing) - `clique_solver.cpp`

Ce document décrit en détail les implémentations des algorithmes d'optimisation locale ("Hill Climbing") employés pour résoudre le problème de la clique maximum au sein du fichier `clique_solver.cpp`.

L'idée fondamentale du Hill Climbing (ou méthode de la plus forte pente) dans le cadre de ce problème est d'améliorer itérativement une solution existante (une clique) en y apportant de petites modifications via l'exploration d'un "voisinage". À chaque étape, on tente d'augmenter la taille de la clique jusqu'à atteindre un optimum local, c'est-à-dire une situation où aucune des modifications testées ne permet d'agrandir la clique.

Un point central partagé par tous les algorithmes est le **vecteur de `candidates`**. Ce vecteur représente l'ensemble des sommets restants du graphe qui sont directement connectés à _tous_ les sommets constituant la clique actuelle. Ainsi, n'importe quel candidat peut être ajouté à la clique actuelle tout en conservant une clique valide. La fonction `initialize_candidates(...)` s'occupe de remplir cette liste lors de toute modification majeure de la clique et `intersect_neighbors(...)` effectue des filtres rapides un par un lors d'ajouts mineurs.

---

## 1. Descente Gloutonne Classique : `greedy_descent_n1` (Voisinage N1)

### L'idée globale

Il s'agit de la méthode originelle (voisinage $N_1$) la plus simple et rapide. Elle consiste simplement à étendre la clique actuelle sommet par sommet en sélectionnant un seul nœud pertinent, à la manière d'un algorithme glouton (greedy).

### Fonctionnement du programme

1. L'algorithme prend en paramètre une clique initiale (`initial_clique`), qui peut d'ailleurs être complètement vide pour démarrer ex nihilo, et initialise ses `candidates`.
2. Une boucle s'exécute **tant que la liste de candidats n'est pas vide** :
   - À chaque itération, l'algorithme fait appel à l'heuristique (ou stratégie) fournie en paramètre via l'interface `s.select(...)`. Typiquement, une stratégie peut consister à choisir le candidat disposant du degré résiduel maximal (le candidat connecté au plus grand nombre d'autres candidats).
   - Ce `best_v` est sélectionné et effacé de la pile des candidats potentiels.
   - Ensuite on l'ajoute officiellement à la clique (`clique.push_back(best_v)`).
   - L'espace des candidats se réduit drastiquement : tous les candidats qui ne sont pas connectés à `best_v` perdent leur statut de candidats (`g.intersect_neighbors(candidates, best_v)`).
3. L'algorithme se termine et retourne la clique lorsque son espace de candidats est vide (optimum local pour $+1$).

### Exemple

Imaginons une clique `{1}` et des candidats `{2, 3, 5}`.
Le nœud 2 est connecté à 3, alors que 5 est isolé vis-à-vis de 2 et 3.
L'heuristique (du degré résiduel) privilégiera l'ajout du **nœud 2** car il aura préservé le sommet 3. La clique devient `{1, 2}` et la nouvelle liste de candidats devient `{3}`.

---

## 2. Descente par Paires : `pair_descent_n2` (Voisinage N2)

### L'idée globale

L'ajout sommet par sommet (N1) se heurte rapidement à des optima difficiles à franchir en fonction des choix faits plus tôt. Le voisinage $N_2$ propose alors d'ajouter **deux sommets simultanément** plutôt qu'un seul. Cette approche permet une évaluation conjointe (le choix mutualisé) d'une paire d'arrêtes pour optimiser l'espace des solutions encore valides.

### Fonctionnement du programme

1. Initialisation identique par rapport à la clique existante et des candidats possibles.
2. Une boucle s'exécute **tant qu'il reste au moins 2 candidats** :
   - On calcule, localement pour cette boucle, les degrés résiduels de tous les candidats disponibles.
   - On imbrique deux boucles `(i, j)` pour explorer toutes les combinaisons possibles de paires de candidats de la liste actuelle.
   - Si un lien existe entre les candidats `i` et `j` (une arête permet de vérifier que la paire entière formera toujours une clique continue) : on calcule le score de cette paire de la façon suivante : `res_degrees[i] + res_degrees[j]`.
   - On garde minutieusement en mémoire la meilleure paire `(best_u, best_v)` rencontrée, ayant ce fameux score (somme des degrés marginaux) maximal et on les ajoute ensemble à la clique.
   - Chaque ajout vient de nouveau filtrer les candidats via `intersect_neighbors`.
3. Si la boucle s'arrête (ex. plus de paires connectées valides) mais que _`use_fallback`_ est vrai, l'algorithme "termine le travail" en basculant en simple glouton `greedy_descent_n1` histoire d'ajouter éventuellement le dernier candidat esseulé qui aurait pu rester.

---

## 3. Descente par Triplets ou Échanges : `triple_descent_n3` (Voisinage N3)

### L'idée globale

C'est un des voisinages le plus complexe. Il combine la dynamique asymétrique d'ajouter plusieurs sommets en une fois (jusqu'à 3 simultanément) tout en incorporant une mécanique vitale de **"retournement des choix de sélection"** ("échange 1-pour-2"). Cela permet de débloquer de vrais faux-positifs où enlever un "mauvais sommet" initial permettrait dans un sous-arbre inexploré, d'y rajouter deux sommets "bons" (donnant ainsi un gain net de taille $+1$ à la clique).

### Fonctionnement du programme

C'est basé sur un système de conditionnements successifs au travers d'une grande boucle `while(improved)`. L'algorithme effectue 4 passes différentes dans l'ordre par itération :

1. **Tentative +3 (Triplets)** : Traverse trois boucles `(i, j, k)` afin de trouver s'il existe carrément un triangle direct de nœuds dans les candidats (connectés tous les trois entre-eux). Pareillement, la meilleure combinaison de scores est privilégiée pour ajouter ces trois éléments et on retourne direct à l'itération suivante.
2. _(Si +3 échoue)_ **Tentative +2 (Paires)** : Essaye exactement la même mécanique de paire qu'expliquée dans `pair_descent_n2`. S'il y parvient, l'itération redémarre.
3. _(Si +2 échoue)_ **Tentative +1 (Simple)** : Tente d'ajouter un sommet selon l'heuristique simple `s.select()` via N1. Si cela réussi, il ajoute et redémarre l'itération.
4. _(Si aucun ajout passif est possible)_ **Échange 1 pour 2** :
   - Si les étapes 1, 2, et 3 n'ont rien donné, on se retrouve bloqué.
   - Le programme boucle en parcourant méticuleusement **chaque sommet faisant partie de la clique de bout-en-bout**.
   - Parité de test : il supprime temporairement ce sommet `v_removed` issu de l'index `i`. L'impact immédiat c'est que les candidats se retrouvent rafraichit (`initialize_candidates` complet est nécessaire) car un blocage vient d'être sauté.
   - L'algo recherche une potentielle paire d'éléments `(j, k)` dans ces nouveaux candidats autorisés pour les ajouter.
   - S'il trouve cette paire, _l'échange complet de taille gagne +1 est ratifié!_ La variable `improved=true` s'empare du contrôle et le flux global utilise un `goto next_iteration` pour recracher cette clique au tout début de sa longue routine.

---

## 4. Ruin and Recreate (Méthode de métaheuristique - Diversification)

### L'idée globale

Il s'agit moins d'un simple calcul de voisinage (descente) que d'une surcouche macro (algorithme itératif diversifié). Les descentes N1, N2, et N3 terminent inévitablement dans un point de non-retour un jour appelé _optimum local_. Pour trouver l'optimum "global" (la véritable clique de taille max de l'intégralité du graphe), l'approche consiste simplement à : "Démolir, une petite portion, puis recréer d'une nouvelle manière".

### Fonctionnement du programme

1. Établit une `best_clique` en réalisant une première grosse phase gloutonne (`greedy_descent_n1`).
2. Pour un certain budget de temps ou d'itérations (`iterations` données en paramètre) :
   - **Étape Ruin** : On mélange complétement ("shuffle") de manière aléatoire tous les éléments formant notre clique. Puis, la clique est brutalement raccourcie d'après un ratio dynamique donné par `ruin_percent` (si ce pourcentage est à 30, il réduira la taille du buffer pour conserver que 70% de ses sommets antérieurs au hasard et supprime les vieux résidus).
   - **Étape Recreate** : L'état d'infirmité passe à nouveau sous le traitement du `greedy_descent_n1` en gardant sa base. Comme les anciens nœuds ont fait de l'espace, le glouton explorera forcement une toute nouvelle trajectoire des candidats !
   - Si la taille dépasse notre ancien record (`best_clique`), on remplace la solution optimale mémorisée à ce jour.
3. En sortie du programme, la très performante `best_clique` aura navigué plusieurs zones de recherche distinctes. L'exploration a alors une grande marge d'avoir percé le mur sans faire appel à un calcul complet faramineux.
