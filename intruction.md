# Proposition pour l'implémentation de la Phase N2 (Hill Climber)

Ce document résume les stratégies d'exploration du voisinage pour l'algorithme de recherche locale appliqué au problème de la Clique Maximum (MCP). Le voisinage **N2** se définit par l'exécution de précisément **deux actions** élémentaires sur le graphe.

## 1. Extension du voisinage : Croissance (Ajout + Ajout)

L'objectif est d'augmenter la taille de la clique de +2 en deux opérations d'ajout.

### Stratégies de sélection des candidats :

- **Best Improvement (Heuristique du degré résiduel)** :
  - On sélectionne la paire de candidats connectés `(u, v)` ayant la **somme maximale des degrés résiduels**.
- **First Improvement (Premier couple trouvé)** :
  - Dès qu'un couple de candidats connectés est trouvé, on l'ajoute immédiatement.

## 2. Voisinage N2 : Exploration (Retrait + Ajout)

Cette opération consiste en un **swap 1-pour-1**. La taille de la clique reste constante, mais sa composition change pour débloquer de futures opportunités.

### Stratégie de Swap (1-pour-1) :

- **Action** : On retire un sommet `w` de la clique actuelle et on ajoute un sommet `u` extérieur.
- **Condition** : `u` doit être connecté à tous les sommets de la clique restante (`Clique \ {w}`).
- **Sélection** : On choisit un couple `(w, u)` tel que le nouveau sommet `u` possède un **degré résiduel supérieur** à celui de `w`.
- **Intérêt** : Permet de sortir d'un optimum local en "échangeant" un sommet peu connecté contre un sommet ayant plus de potentiel d'expansion.

## 3. Stratégies de diversification (Optimal local)

Si aucune opération N2 n'est possible (ni ajout ni swap améliorant le potentiel) :

- **Repli N1 (Addition simple)** : On tente d'ajouter un seul sommet si possible (+1 opération).
- **Perturbation Partielle (50% Destruction)** :
  - On retire aléatoirement 50% des sommets de la clique actuelle pour explorer une nouvelle zone.
- **Redémarrage itératif (Multi-start)** :
  - On répète le processus en conservant la **meilleure clique globale** trouvée.

---

### Recommandation d'implémentation :

Privilégier systématiquement l'**Ajout + Ajout** pour croître. Utiliser le **Retrait + Ajout** uniquement comme levier pour débloquer la croissance lorsque la clique est temporairement saturée.

construit de le code manière à pouvoir séparer chaque stratégie dans des fonctions distinctes.
Utilise les fonctions qui sont prédéfinies dans le fichier utils.cpp. Eviter la methode isClique() mais favoriser la methode intersection().
