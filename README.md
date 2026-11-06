# Neuron Forge

> Jeu pédagogique en vue à la première personne sous **Unreal Engine 5.6** où l'on construit, entraîne et valide un réseau de neurones à l'intérieur d'un laboratoire.

Projet réalisé pour la matière **Réalité étendue et interactions multimodales** à l'**ENSIM** (Le Mans Université).

---

## Sommaire

1. [Concept pédagogique](#concept-pédagogique)
2. [Gameplay](#gameplay)
3. [Notions du cours mises en œuvre](#notions-du-cours-mises-en-œuvre)
4. [Architecture technique](#architecture-technique)
5. [Installation, ouverture et compilation](#installation-ouverture-et-compilation)
6. [Scripts Python de l'éditeur](#scripts-python-de-léditeur)
7. [État d'avancement](#état-davancement)
8. [Roadmap](#roadmap)

---

## Concept pédagogique

Le deep learning est souvent enseigné de façon abstraite : des équations, des courbes de perte, du code. **Neuron Forge** rend ces notions *tangibles* en plaçant le joueur dans un laboratoire où le réseau de neurones devient un objet physique qu'il assemble de ses mains.

Le joueur doit **concevoir l'architecture d'un modèle** (nombre de couches cachées, nombre de neurones par couche, fonction d'activation, taux d'apprentissage), **lancer l'entraînement** et **observer** en temps réel la perte diminuer, les poids évoluer et les neurones s'allumer. La porte du laboratoire ne s'ouvre que si le modèle atteint la **précision cible**.

Ce que le joueur apprend en jouant :

- **Pourquoi la non-linéarité est indispensable** : sans couche cachée ou avec une activation linéaire, le réseau ne peut pas résoudre XOR, quel que soit le nombre d'epochs.
- **L'effet de la capacité du modèle** : trop peu de neurones, le modèle stagne ; assez de neurones, il converge.
- **L'influence des hyperparamètres** : un taux d'apprentissage trop faible ralentit l'entraînement, trop fort le rend instable.
- **Le rôle des fonctions d'activation** : ReLU, Sigmoid, Tanh et Linear n'ont pas le même comportement, ce que la couleur et l'intensité des neurones rendent visible.

Le premier niveau (MVP) utilise le dataset **XOR**, le problème historique qui a montré les limites du perceptron simple.

| x₁ | x₂ | XOR |
|:--:|:--:|:---:|
| 0  | 0  |  0  |
| 0  | 1  |  1  |
| 1  | 0  |  1  |
| 1  | 1  |  0  |

---

## Gameplay

1. **Entrée dans le laboratoire** : le joueur arrive dans une salle fermée par une porte. Un panneau lumineux (Text Render) affiche la mission : « Entraînez un modèle qui résout XOR avec 100 % de précision ».
2. **Construction du réseau** sur l'**holotable** centrale, où le réseau est représenté en 3D : chaque neurone est une sphère émissive et chaque poids un lien coloré selon son signe et épais selon son amplitude. Les panneaux de configuration permettent de :
   - ajouter ou retirer une couche cachée (jusqu'à 3),
   - augmenter ou diminuer le nombre de neurones d'une couche (1 à 8),
   - changer la fonction d'activation (ReLU, Sigmoid, Tanh, Linear),
   - régler le taux d'apprentissage.
3. **Lancement de l'entraînement** : le joueur marche sur le **bouton au sol**. L'entraînement se déroule progressivement (quelques epochs par frame) pour que l'on voie le réseau apprendre.
4. **Suivi en direct** : l'écran d'état affiche l'architecture, l'epoch courante, la perte, la précision et les prédictions sur les 4 exemples XOR.
5. **Validation** : si la précision cible est atteinte, un **son de validation** retentit et la **porte du labo s'ouvre** avec une animation. Sinon, le joueur modifie son architecture et réessaie.
6. **Sortie** : franchir la porte déclenche la zone de fin de niveau.

**Contrôles (PC)** : `ZQSD`/`WASD` pour se déplacer, la souris pour regarder, `Espace` pour sauter, `E` ou clic gauche pour interagir avec l'objet visé.

---

## Notions du cours mises en œuvre

| Notion | Utilisation dans Neuron Forge |
|---|---|
| **Trigger Box Collision** | Le bouton au sol détecte le joueur par overlap pour lancer l'entraînement ; une zone de fin de niveau détecte la sortie du labo. |
| **Porte animée** | La porte du labo s'ouvre via une timeline (courbe d'animation) en C++ lorsque la précision cible est atteinte. |
| **Bouton au sol** | Plaque de pression qui s'enfonce et s'illumine quand le joueur marche dessus, puis démarre l'entraînement. |
| **Text Render** | Affichage dans le monde 3D de la mission, de l'état de l'entraînement (epoch, perte, précision) et des libellés des panneaux. |
| **Son de validation** | Son joué à la réussite du modèle, au moment de l'ouverture de la porte. |
| **Materials émissifs** | Les neurones brillent selon leur fonction d'activation (couleur) et leur valeur d'activation (intensité), grâce à des Material Instances dynamiques. |
| **Interactions multimodales** | Une interface d'interaction commune (`INFInteractable`) sépare l'objet interactif de la manière d'interagir (visée + touche sur PC, et plus tard main/contrôleur en VR). |

---

## Architecture technique

### Principe

- **Toute la logique est en C++** : le réseau de neurones, l'entraînement, les interactions, le gameplay et l'interface.
- **Le contenu est généré par des scripts Python** exécutés dans l'éditeur (plugin *Python Editor Script*) : le niveau du labo, le placement des acteurs, les materials et les Blueprints enfants des classes C++.
- **Le personnage reprend celui du template First Person** : mesh, Anim Blueprint et assets Enhanced Input.

### Modules et classes C++ (module `ProjetNeuro`)

| Domaine | Classes | Rôle |
|---|---|---|
| Réseau de neurones | `FNFNeuralNetwork`, `FNFDenseLayer`, `ENFActivation` | Perceptron multicouche en C++ pur : propagation avant, rétropropagation, SGD, initialisation Xavier/He. |
| Données | `FNFDataset` | Datasets d'entraînement (XOR pour le MVP). |
| Entraînement | `UNFTrainerComponent` | Entraînement réparti sur plusieurs frames, avec des événements de progression et de réussite. |
| Interaction | `INFInteractable`, `UNFInteractorComponent` | Interface commune à tous les objets interactifs, et composant qui interagit (line trace depuis la caméra sur PC, composant de main en VR plus tard). |
| Joueur | `ANFCharacter`, `ANFPlayerController`, `ANFGameMode`, `ANFHUD` | Personnage FPS en C++ (repris du template), contrôleur, règles du jeu, réticule et indications à l'écran. |
| Labo | `ANFNetworkBuilder` | Holotable : configuration et visualisation 3D du réseau. |
| Labo | `ANFConfigButton` | Boutons muraux de configuration (couches, neurones, activation, taux d'apprentissage). |
| Labo | `ANFFloorButton` | Bouton au sol (Box Collision) qui lance l'entraînement. |
| Labo | `ANFStatusDisplay` | Écran d'état (Text Render). |
| Labo | `ANFLabDoor` | Porte animée et son de validation. |
| Labo | `ANFExitTrigger` | Zone de fin de niveau (Box Collision). |

### Arborescence

```
ProjetNeuro/
├── ProjetNeuro.uproject     Projet UE 5.6 (module C++ + plugins Python)
├── Source/
│   ├── ProjetNeuro.Target.cs
│   ├── ProjetNeuroEditor.Target.cs
│   └── ProjetNeuro/         Code C++ du jeu
├── Content/
│   ├── Python/              Scripts Python de génération du contenu
│   ├── FirstPerson/         Personnage et assets du template
│   └── NeuronForge/         Contenu généré (niveau, materials, Blueprints)
└── Config/                  Configuration du projet
```

---

## Installation, ouverture et compilation

### Prérequis

- **Unreal Engine 5.6** (testé avec 5.6.1)
- **Visual Studio 2022** avec les charges de travail *Développement Desktop en C++* et *Développement de jeux en C++*
- **Git** et **Git LFS** (les fichiers `.uasset` et `.umap` sont stockés avec LFS)

### Récupérer le projet

```bash
git lfs install
git clone https://github.com/adam-hassen/neuron-forge.git
```

Si le dépôt a été cloné sans LFS, récupérez les assets avec `git lfs pull`.

### Compiler

**Option 1 : depuis l'Explorateur Windows**
1. Clic droit sur `ProjetNeuro.uproject` → *Generate Visual Studio project files*.
2. Ouvrez `ProjetNeuro.sln`, choisissez la configuration **Development Editor** / **Win64** et compilez.

**Option 2 : en ligne de commande** (l'éditeur doit être fermé)

```bat
"<UE_5.6>\Engine\Build\BatchFiles\Build.bat" ProjetNeuroEditor Win64 Development -Project="<chemin>\ProjetNeuro.uproject" -WaitMutex
```

`<UE_5.6>` désigne le dossier d'installation du moteur, par exemple `C:\Program Files\Epic Games\UE_5.6`.

### Ouvrir et jouer

Double-cliquez sur `ProjetNeuro.uproject`. Si l'éditeur propose de recompiler des modules manquants, acceptez. Lancez ensuite le niveau du labo avec **Play**.

---

## Scripts Python de l'éditeur

Les scripts se trouvent dans `Content/Python/` et sont exécutés en mode *headless* (sans interface, **éditeur fermé**) :

```bat
"<UE_5.6>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<chemin>\ProjetNeuro.uproject" -run=pythonscript -script="<chemin absolu du script .py>" -unattended -nullrhi
```

| Script | Rôle |
|---|---|
| `nf_check_env.py` | Vérifie que Python fonctionne et voit les assets du projet (rapport dans `Saved/nf_check_env.txt`). |

Ils peuvent aussi être lancés depuis l'éditeur ouvert : *Outils → Exécuter un script Python*.

---

## État d'avancement

| Étape | Contenu | État |
|---|---|---|
| 0 | Dépôt Git + LFS, conversion du template en projet C++, activation de Python | ✅ Terminé |
| 1 | Réseau de neurones en C++ et test automatisé sur XOR | ⏳ En cours |
| 2 | Interface d'interaction commune, personnage C++, HUD | 🔲 À faire |
| 3 | Acteurs du labo : holotable, boutons, bouton au sol, écran d'état, porte, son | 🔲 À faire |
| 4 | Génération du contenu en Python : materials, son, Blueprints, niveau `L_Lab` | 🔲 À faire |
| 5 | Test de bout en bout et finitions | 🔲 À faire |

---

## Roadmap

**MVP : niveau XOR jouable de bout en bout**
- Construction du réseau, entraînement visible, validation par la porte.

**Après le MVP**
- **Nouveaux niveaux et datasets** : cercles concentriques, spirales, classification multi-classes, chacun avec sa propre précision cible.
- **Visualisation de la frontière de décision** sur un écran du labo.
- **Optimiseurs supplémentaires** (Momentum, Adam) et choix de la fonction de perte.
- **Mode réalité virtuelle** : interactions à la main ou au contrôleur grâce à l'interface `INFInteractable` déjà en place.
- **Interface UMG** pour le menu principal et les explications pédagogiques.
- **Retour pédagogique** : messages d'aide quand le modèle échoue (par exemple « Essayez d'ajouter une couche cachée »).
