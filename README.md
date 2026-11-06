# Neuron Forge

> Jeu pédagogique en vue à la première personne sous **Unreal Engine 5.6** où l'on construit, entraîne et valide un réseau de neurones à l'intérieur d'un laboratoire.

Projet réalisé pour la matière **Réalité étendue et interactions multimodales** à l'**ENSIM** (Le Mans Université).

![Le laboratoire Neuron Forge après la réussite du modèle](Docs/Images/NF_Holotable.png)

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

| Vue d'ensemble du labo | Console de configuration | Porte ouverte après validation |
|:---:|:---:|:---:|
| ![Entrée du labo](Docs/Images/NF_Entree.png) | ![Console](Docs/Images/NF_Console.png) | ![Porte](Docs/Images/NF_Porte.png) |

**Contrôles (PC)** : `ZQSD`/`WASD` pour se déplacer, la souris pour regarder, `Espace` pour sauter, `E` ou clic gauche pour interagir avec l'objet visé. À la manette : stick gauche/droit, `A` pour sauter, `X` pour interagir.

L'objet visé est mis en surbrillance et le réticule affiche l'action possible, par exemple « [E] Ajouter un neurone ».

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
| Réseau de neurones | `FNFNeuralNetwork`, `FNFDenseLayer`, `FNFLayerConfig`, `ENFActivation` | Perceptron multicouche en C++ pur : propagation avant, rétropropagation, SGD avec mélange des exemples, initialisation Xavier/He, perte choisie automatiquement (entropie croisée binaire avec une sortie Sigmoid, MSE sinon). |
| Données | `FNFDataset` | Datasets d'entraînement (XOR pour le MVP). |
| Entraînement | `UNFTrainerComponent` | Entraînement réparti sur les frames (epochs par seconde réglables) ; états Idle / Training / Succeeded / Failed ; cible : précision 100 % **et** perte < 0,1 ; échec après 2000 epochs ; événements `OnStateChanged` et `OnProgress`. |
| Interaction | `INFInteractable` | Interface commune à tous les objets interactifs : `CanInteract`, `Interact`, `OnFocusBegin`, `OnFocusEnd`, `GetInteractionPrompt`. L'objet ne sait pas comment on interagit avec lui. |
| Interaction | `UNFInteractorComponent` | Ce qui interagit : un rayon depuis la caméra (PC) ou depuis un composant quelconque via `SetTraceSource` (contrôleur VR), et `InteractWith` pour un contact direct (main VR). |
| Interaction | `ANFInteractableActor` | Acteur interactif de base : mesh, texte d'aide, surbrillance (paramètre de material `Highlight` + Custom Depth), événement `OnInteracted`. |
| Joueur | `ANFCharacter` | Personnage FPS en C++ reprenant la configuration du template (bras FP, corps en ombre, caméra sur la tête, FOV first person) et ses assets ; ajoute l'action « Interagir ». |
| Joueur | `ANFPlayerController`, `ANFPlayerCameraManager`, `ANFHUD` | Contrôleur, limites de caméra du template, réticule, texte d'aide et messages temporaires dessinés au Canvas. |
| Règles | `ANFGameMode` | Associe le personnage, le contrôleur et le HUD du jeu. |
| Labo | `ANFNetworkBuilder` | Holotable : configuration du réseau (actions `ENFConfigAction`), conseils pédagogiques, visualisation 3D en instances (neurones = sphères colorées selon l'activation et lumineuses selon leur valeur, poids = cylindres bleus/rouges selon le signe et épais selon l'amplitude), défilement des exemples XOR. |
| Labo | `ANFConfigButton` | Bouton mural : applique une action sur une couche ; grisé si l'action est impossible (couche absente, limite atteinte, entraînement en cours) ; texte d'aide dynamique. |
| Labo | `ANFFloorButton` | Bouton au sol : trigger Box Collision qui détecte le joueur, plaque qui s'enfonce et s'illumine, lance l'entraînement. |
| Labo | `ANFStatusDisplay` | Écran d'état en Text Render : mission, architecture, epoch, perte, précision, prédictions sur les 4 cas, conseil. |
| Labo | `ANFLabDoor` | Porte coulissante animée par une timeline (`FTimeline` + courbe ease-in-out) ; s'ouvre à la réussite avec le son de validation ; voyant rouge → vert. |
| Labo | `ANFExitTrigger` | Zone de fin de niveau (Box Collision) derrière la porte. |

**Parcours pédagogique prévu** : le réseau de départ n'a pas de couche cachée et échoue (« XOR n'est pas linéairement séparable ») ; la couche ajoutée est Linear et échoue aussi (« des couches Linear empilées restent linéaires ») ; en passant à Tanh, ReLU ou Sigmoid, le modèle réussit et la porte s'ouvre.

### Arborescence

```
ProjetNeuro/
├── ProjetNeuro.uproject     Projet UE 5.6 (module C++ + plugins Python)
├── Source/
│   ├── ProjetNeuro.Target.cs
│   ├── ProjetNeuroEditor.Target.cs
│   └── ProjetNeuro/         Code C++ du jeu
│       ├── Core/            GameMode
│       ├── Interaction/     Interface commune, interacteur, acteur interactif de base
│       ├── NeuralNetwork/   Réseau de neurones, activations, datasets
│       ├── Lab/             Holotable, boutons, bouton au sol, écran, porte, sortie
│       ├── Player/          Personnage, contrôleur, caméra, HUD
│       ├── Training/        Composant d'entraînement
│       └── Tests/           Tests d'automatisation
├── Content/
│   ├── Python/              Scripts Python de génération du contenu
│   ├── FirstPerson/         Personnage et assets du template
│   └── NeuronForge/         Contenu généré par les scripts
│       ├── Audio/           Effets sonores (S_NF_*)
│       ├── Blueprints/      Blueprints enfants des classes C++ (BP_NF_*)
│       ├── Maps/            Niveau du labo L_Lab
│       └── Materials/       Materials et instances (M_NF_*, MI_NF_*)
├── Docs/Images/             Captures d'écran du README
├── RawAssets/Audio/         Sons WAV générés (source de l'import)
└── Config/                  Configuration du projet (carte et GameMode par défaut : L_Lab, BP_NF_GameMode)
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

> **Projet sur un lecteur réseau ?** Sur certains partages réseau, la date de modification des fichiers n'est pas mise à jour de façon fiable : Unreal Build Tool peut alors annoncer `Result: Succeeded` sans rien recompiler. Si une modification ne semble pas prise en compte, vérifiez que la sortie contient des lignes `[n/m] Compile`, ou mettez à jour la date des fichiers modifiés (`touch` sous Git Bash) avant de recompiler.

### Ouvrir et jouer

Double-cliquez sur `ProjetNeuro.uproject`. Si l'éditeur propose de recompiler des modules manquants, acceptez. L'éditeur s'ouvre sur le niveau du labo `L_Lab` (`Content/NeuronForge/Maps`) : lancez-le avec **Play**.

---

## Scripts Python de l'éditeur

Les scripts se trouvent dans `Content/Python/` et sont exécutés en mode *headless* (sans interface, **éditeur fermé**) :

```bat
"<UE_5.6>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<chemin>\ProjetNeuro.uproject" -run=pythonscript -script="<chemin absolu du script .py>" -unattended -nullrhi
```

**Pour régénérer tout le contenu du jeu** (sons, materials, Blueprints, niveau) :

```bat
"<UE_5.6>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<chemin>\ProjetNeuro.uproject" -run=pythonscript -script="<chemin>\Content\Python\nf_build_all.py" -unattended -nullrhi
```

Le rapport est écrit dans `Saved/nf_build_report.txt`. `nf_validate_content.py` vérifie ensuite, dans un nouveau processus, que tout a bien été enregistré (`Saved/nf_validate_report.txt`).

| Script | Rôle |
|---|---|
| `nf_build_all.py` | Point d'entrée : sons → materials → Blueprints → niveau `L_Lab`, avec rapport. |
| `nf_audio.py` | Synthèse des 7 effets sonores en Python pur (WAV dans `RawAssets/Audio/`) : validation (arpège do-mi-sol-do), clic, bouton au sol, lancement, échec, porte, fin de niveau. |
| `nf_assets.py` | Materials (`M_NF_Instanced` lu par instance, `M_NF_Highlight` avec paramètre `Highlight`, `M_NF_Surface`) et leurs 11 instances ; import des sons ; Blueprints enfants `BP_NF_*` des classes C++ avec materials et sons assignés. |
| `nf_level.py` | Construction du niveau `L_Lab` : salle 16 × 14 m, couloir de sortie, éclairage, post-process, holotable, écran d'état, bouton au sol, console de 13 boutons, panneau « mode d'emploi », porte et zone de sortie, GameMode du niveau. |
| `nf_validate_content.py` | Contrôle du contenu enregistré : materials et sons des Blueprints, acteurs du niveau et leurs références, GameMode. |
| `nf_check_env.py` | Vérifie que Python fonctionne, que les classes C++ du jeu sont chargées et que le personnage a bien récupéré les assets du template (rapport dans `Saved/nf_check_env.txt`). |
| `nf_dump_template.py` | Inventaire des Blueprints du template First Person : composants, assets, réglages et mappings d'entrée (rapport dans `Saved/nf_dump_template.txt`). |

Ils peuvent aussi être lancés depuis l'éditeur ouvert : *Outils → Exécuter un script Python*.

---

## Tests automatisés

Le réseau de neurones est couvert par des tests d'automatisation Unreal (`Source/ProjetNeuro/Tests/`), visibles dans *Outils → Session Frontend → Automation* sous `NeuronForge`. En ligne de commande (éditeur fermé) :

```bat
"<UE_5.6>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<chemin>\ProjetNeuro.uproject" -ExecCmds="Automation RunTests NeuronForge; Quit" -unattended -nullrhi -TestExit="Automation Test Queue Empty"
```

| Test | Ce qu'il vérifie |
|---|---|
| `Activations` | Valeurs des fonctions d'activation, stabilité numérique de Sigmoid, cycle des activations. |
| `Determinism` | Même graine ⇒ mêmes poids ; nombre de paramètres ; perte choisie automatiquement. |
| `GradientCheck` | Les gradients de la rétropropagation correspondent aux dérivées numériques (écart < 0,0002). |
| `XORSolved` | `2 - 4 Tanh - 1 Sigmoid` résout XOR (100 %). |
| `XORLinearFails` | Sans couche cachée, ou avec une couche cachée Linear, XOR reste à 50 % : **la non-linéarité est indispensable**. |
| `Interaction.Interactor` | Dans un monde de test : rayon, distance maximale, focus et surbrillance, interaction, objet désactivé, source de visée de type contrôleur VR, interaction directe sans rayon. |
| `Lab.BuilderEditing` | Actions de configuration et leurs limites, verrouillage pendant l'entraînement, bouton lié à une couche absente puis présente. |
| `Lab.FloorButtonStartsTraining` | Un pawn qui entre dans la Box Collision du bouton au sol lance l'entraînement ; le bouton se relâche quand il repart. |
| `Lab.XORSuccessOpensDoor` | Bout en bout : `2 - 4 Tanh - 1 Sigmoid` réussit (57 epochs) et la porte s'ouvre complètement. |
| `Lab.LinearModelKeepsDoorClosed` | Sans couche cachée : échec, porte fermée, conseil « Ajoutez une couche », configuration de nouveau modifiable. |

**Test de fumée du niveau réel** (`Smoke.LabPlaythrough`, contexte jeu uniquement) : il lance le jeu sur `L_Lab` et rejoue le parcours du joueur. Le joueur vise et actionne les boutons de la console par le vrai rayon d'interaction et règle une couche Tanh. Il monte ensuite sur le bouton au sol, attend que le modèle soit validé et que la porte s'ouvre, puis entre dans la zone de sortie. Résultat actuel : modèle validé en 57 epochs, porte ouverte en 2,1 s.

```bat
"<UE_5.6>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<chemin>\ProjetNeuro.uproject" -game -nullrhi -nosound -ExecCmds="Automation RunTests NeuronForge.Smoke; Quit" -TestExit="Automation Test Queue Empty"
```

**Captures de contrôle visuel** (`Smoke.Screenshots`) : il prend 5 vues du labo (entrée, holotable, console, mode d'emploi, porte) dans `Saved/Screenshots/NF_*.png`. Il demande un vrai rendu : remplacer `-nullrhi` par `-RenderOffscreen -ResX=1280 -ResY=720` dans la commande ci-dessus.

**Mesures de calibrage** (graine 42, arrêt à 100 % de précision et perte < 0,1) :

| Architecture | Taux d'apprentissage | Résultat |
|---|:---:|---|
| 2 - 1 Sigmoid (sans couche cachée) | 0,5 | ❌ bloqué à 50 % |
| 2 - 8 Linear - 1 Sigmoid | 0,5 | ❌ bloqué à 50 % |
| 2 - 2 Tanh - 1 Sigmoid | 0,5 | ❌ bloqué à 50 % (trop peu de neurones avec cette initialisation) |
| 2 - 4 Tanh - 1 Sigmoid | 0,5 | ✅ 55 epochs |
| 2 - 4 Tanh - 4 Tanh - 1 Sigmoid | 0,3 | ✅ 55 epochs |
| 2 - 8 ReLU - 1 Sigmoid | 0,1 | ✅ 132 epochs |
| 2 - 4 Sigmoid - 1 Sigmoid | 1,0 | ✅ 171 epochs |
| 2 - 4 Tanh - 1 Sigmoid | 0,01 | ✅ 2067 epochs (taux trop faible : très lent) |

---

## État d'avancement

| Étape | Contenu | État |
|---|---|---|
| 0 | Dépôt Git + LFS, conversion du template en projet C++, activation de Python | ✅ Terminé |
| 1 | Réseau de neurones en C++ et test automatisé sur XOR | ✅ Terminé |
| 2 | Interface d'interaction commune, personnage C++, HUD | ✅ Terminé |
| 3 | Acteurs du labo : holotable, boutons, bouton au sol, écran d'état, porte, son | ✅ Terminé |
| 4 | Génération du contenu en Python : materials, son, Blueprints, niveau `L_Lab` | ✅ Terminé |
| 5 | Test de bout en bout et finitions | ⏳ Test dans l'éditeur en cours |

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
