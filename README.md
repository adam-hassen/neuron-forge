# Neuron Forge

> Jeu pédagogique en vue à la première personne sous **Unreal Engine 5.6** où l'on construit, entraîne et valide un réseau de neurones à l'intérieur d'un laboratoire.

Projet réalisé pour la matière **Réalité étendue et interactions multimodales** à l'**ENSIM** (Le Mans Université).

![Le grand hall de Neuron Forge : la Forge et l'hologramme géant](Docs/Images/NF_Hologramme.png)

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

Le joueur **assemble de ses mains l'architecture d'un modèle**, comme un puzzle : il porte des pièces physiques jusqu'à la Forge (un module = une couche cachée, une orbe = un neurone, un cristal = une fonction d'activation, un levier = le taux d'apprentissage). Il **lance l'entraînement** et **observe** sur un hologramme géant la perte diminuer, les poids évoluer et les signaux circuler. La porte du laboratoire ne s'ouvre que si le modèle atteint la **précision cible**.

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

1. **Briefing de mission** : au lancement, un écran d'introduction en 4 pages (jeu en pause) présente le contexte, le problème XOR, les 3 étapes de la mission et les commandes. Il se rappelle à tout moment avec `H`. Pendant la partie, un panneau d'objectifs en haut à gauche coche les étapes et compte les essais.
2. **Entrée dans le laboratoire** : le joueur arrive dans une salle fermée par une porte. L'écran d'état (Text Render) affiche la mission : « Entraînez un modèle qui résout XOR avec 100 % de précision ».
3. **Assemblage du réseau sur la Forge** : dans le grand hall, l'atelier propose trois établis de pièces que le joueur attrape (`E`), porte et emboîte :
   - un **module de couche**, inséré dans l'une des 3 fentes de la Forge, ajoute une couche cachée ;
   - chaque **orbe-neurone** posée autour d'un module (jusqu'à 8) ajoute un neurone à cette couche ;
   - un **cristal d'activation** planté sur le module choisit sa fonction (ReLU orange, Tanh violet, Sigmoid vert, Linear bleu ; sans cristal, la couche est Linear) ;
   - le **levier** fait défiler le taux d'apprentissage (0,01 → 2).

   La Forge lit l'assemblage en continu et l'**hologramme géant** reconstruit le réseau en direct : chaque neurone est une sphère émissive, chaque poids un lien coloré selon son signe et épais selon son amplitude. Pour modifier le réseau, il suffit de retirer ou de déplacer une pièce. Pendant l'entraînement, l'assemblage est verrouillé.
4. **Lancement de l'entraînement** : le joueur marche sur le **bouton au sol**. L'entraînement se déroule progressivement (quelques epochs par frame) pour que l'on voie le réseau apprendre.
5. **Suivi en direct** : l'écran d'état affiche l'architecture, l'epoch courante, la perte, la précision et les prédictions sur les 4 exemples XOR.
6. **Validation** : si la précision cible est atteinte, un **son de validation** retentit et la **porte du labo s'ouvre** avec une animation. Sinon, le joueur modifie son architecture et réessaie.
7. **Sortie** : franchir la porte déclenche l'écran « Mission accomplie » : bilan (architecture, epochs, essais, durée), leçon à retenir, boutons Rejouer / Quitter.

| Briefing : accueil | Briefing : la mission | Écran de fin |
|:---:|:---:|:---:|
| ![Briefing](Docs/Images/NF_Intro.png) | ![Mission](Docs/Images/NF_IntroMission.png) | ![Fin](Docs/Images/NF_Fin.png) |

| Pendant l'entraînement | Baies de serveurs et passerelles | Porte du labo |
|:---:|:---:|:---:|
| ![Entraînement](Docs/Images/NF_Entrainement.png) | ![Serveurs](Docs/Images/NF_Serveurs.png) | ![Porte](Docs/Images/NF_Porte.png) |

| Le grand hall | La Forge (module, orbes, cristal) | L'atelier |
|:---:|:---:|:---:|
| ![Hall](Docs/Images/NF_Entree.png) | ![Forge](Docs/Images/NF_Forge.png) | ![Atelier](Docs/Images/NF_Atelier.png) |

**Contrôles (PC)** : `ZQSD`/`WASD` pour se déplacer, la souris pour regarder, `Espace` pour sauter, `E` ou clic gauche pour interagir avec l'objet visé. `H` ou `F1` pour revoir le briefing. À la manette : stick gauche/droit, `A` pour sauter, `X` pour interagir.

L'objet visé est mis en surbrillance et le réticule affiche l'action possible : « [E] Prendre : orbe-neurone », puis, la pièce en main, « [E] Insérer orbe-neurone dans le module » quand un emplacement compatible est à portée (réticule vert).

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
| **Interactions multimodales** | Une interface d'interaction commune (`INFInteractable`) sépare l'objet interactif de la manière d'interagir (visée + touche sur PC, et plus tard main/contrôleur en VR). La prise en main des pièces (`UNFGrabberComponent`) accepte elle aussi une main VR comme source (`SetHoldSource`). |
| **Manipulation d'objets physiques** | Les pièces du puzzle sont des objets physiques que l'on attrape, porte, pose ou emboîte dans des emplacements (sockets) typés. |

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
| Interaction | `UNFGrabberComponent` | Prise en main : la pièce attrapée flotte devant la caméra (ou dans une main VR via `SetHoldSource`) ; posée près d'un emplacement compatible, elle s'y emboîte. |
| Forge | `ANFPiece`, `UNFSocketComponent` | Pièce physique du puzzle (orbe, cristal) et emplacement typé qui l'accueille ; emboîtement par proximité, retour automatique si une pièce tombe hors du hall, verrouillage pendant l'entraînement. |
| Forge | `ANFLayerModule` | Module de couche : 8 emplacements d'orbes en couronne + 1 emplacement de cristal ; résumé flottant « 4 neurones · Tanh ». |
| Forge | `ANFForge` | Rail d'assemblage (entrées, 3 fentes, sortie) : lit les modules insérés et envoie l'architecture à l'hologramme. |
| Forge | `ANFLever` | Levier du taux d'apprentissage : chaque traction passe au cran suivant, cadran lumineux. |
| Interaction | `ANFInteractableActor` | Acteur interactif de base : mesh, texte d'aide, surbrillance (paramètre de material `Highlight` + Custom Depth), événement `OnInteracted`. |
| Joueur | `ANFCharacter` | Personnage FPS en C++ reprenant la configuration du template (bras FP, corps en ombre, caméra sur la tête, FOV first person) et ses assets ; ajoute l'action « Interagir ». |
| Joueur | `ANFPlayerController`, `ANFPlayerCameraManager`, `ANFHUD` | Contrôleur, limites de caméra du template, réticule, texte d'aide et messages temporaires dessinés au Canvas. |
| Règles | `ANFGameMode` | Associe le personnage, le contrôleur et le HUD du jeu. |
| Interface | `SNFIntroWidget` | Briefing de mission animé en 4 pages (Slate) : flou d'arrière-plan, fondus, navigation clavier / souris / manette. |
| Interface | `SNFObjectivesWidget` | Panneau d'objectifs en jeu : étapes cochées au fil de la partie, étape courante en surbrillance, nombre d'essais. |
| Interface | `SNFEndScreenWidget` | Écran « Mission accomplie » : statistiques, leçon pédagogique, Rejouer / Quitter. |
| Interface | `NFUIStyle` | Charte graphique commune : couleurs, polices, cartes arrondies, styles de boutons. |
| Labo | `ANFNetworkBuilder` | Holotable : configuration du réseau (actions `ENFConfigAction`), conseils pédagogiques, visualisation 3D en instances (neurones = sphères colorées selon l'activation et lumineuses selon leur valeur, poids = cylindres bleus/rouges selon le signe et épais selon l'amplitude), défilement des exemples XOR. |
| Labo | `ANFConfigButton` | Bouton mural de configuration (première version du jeu, remplacée par la Forge ; conservé pour les tests et d'autres niveaux). |
| Labo | `ANFFloorButton` | Bouton au sol : trigger Box Collision qui détecte le joueur, plaque qui s'enfonce et s'illumine, lance l'entraînement. |
| Labo | `ANFStatusDisplay` | Écran d'état en Text Render : mission, architecture, epoch, perte, précision, prédictions sur les 4 cas, conseil. |
| Labo | `ANFLabDoor` | Porte coulissante animée par une timeline (`FTimeline` + courbe ease-in-out) ; s'ouvre à la réussite avec le son de validation ; voyant rouge → vert. |
| Labo | `ANFExitTrigger` | Zone de fin de niveau (Box Collision) derrière la porte. |
| Ambiance | `ANFLabAmbience` | Ambiance réactive : couleur et intensité envoyées à la Material Parameter Collection `MPC_NF_Lab` et aux lumières taguées `NFMood` (cyan au repos, orange pulsé pendant l'entraînement, flash vert à la réussite, alerte rouge à l'échec). |
| Ambiance | `ANFAnimatedProp` | Décor animé : rotation, flottement, couronne d'objets en orbite, accélère pendant l'entraînement (hologrammes, cubes de données). |

**Décor du labo** : ambiance sombre de laboratoire de nuit, sol miroir quadrillé de lignes lumineuses, liserés néon sur les murs, les piliers, la porte et les panneaux, baies de serveurs couvertes de LED qui clignotent, socle et anneau holographiques animés au-dessus de l'holotable, cubes de données en orbite, brume volumétrique. Des impulsions lumineuses parcourent les poids du réseau, plus vite pendant l'entraînement. Toute la salle change de couleur selon l'état du modèle.

**Parcours pédagogique prévu** : la Forge vide (aucune couche cachée) échoue (« XOR n'est pas linéairement séparable ») ; un module garni d'orbes mais sans cristal est Linear et échoue aussi (« des couches Linear empilées restent linéaires ») ; avec un cristal Tanh, ReLU ou Sigmoid et assez d'orbes, le modèle réussit et la porte s'ouvre.

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
│       ├── UI/              Briefing, objectifs, écran de fin (Slate)
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
| `nf_assets.py` | Materials (`M_NF_Instanced` lu par instance, `M_NF_Highlight` avec paramètre `Highlight`, `M_NF_Surface`) et leurs instances ; décor animé (`MPC_NF_Lab`, `M_NF_Trim` liserés d'ambiance, `M_NF_FloorGrid` grille lumineuse, `M_NF_ServerLEDs` LED aléatoires, `M_NF_Holo` anneaux holographiques) ; import des sons ; Blueprints enfants `BP_NF_*` avec materials et sons assignés. |
| `nf_level.py` | Construction du niveau `L_Lab`, un grand hall de 45 × 30 m sur 10 m de haut : atelier (3 établis garnis de 4 modules, 20 orbes et 8 cristaux), Forge et levier, hologramme géant, bouton au sol, écran d'état, panneau « mode d'emploi », porte, couloir et zone de sortie ; passerelles, piliers, liserés, baies de serveurs, hologrammes animés ; éclairage d'ambiance, projecteurs, brume volumétrique et acteur d'ambiance. |
| `nf_validate_content.py` | Contrôle du contenu enregistré : materials et sons des Blueprints, acteurs du niveau (Forge, levier, 4 modules, 20 orbes, 8 cristaux) et leurs références, GameMode. |
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
| `Greenhouse.Dataset` | Serre : 160 expériences reproductibles, entrées brutes 0-10 et normalisées -1/1, 25 à 50 % de survies, un peu de bruit (moins de 10 %), règle de la zone de survie. |
| `Greenhouse.LabSuccessOpensDoor` | Holotable en mode serre : dataset et réglages chargés, 5 ReLU réussit en moins de 30 s de jeu, la porte s'ouvre, prédictions ponctuelles cohérentes. |
| `Greenhouse.LinearModelFails` | Sans couche cachée : échec sous 80 % et conseil « ligne droite ». |
| `Greenhouse.Notebook` | Carnet du labo : fond neutre avant l'entraînement, zone verte au centre et rouge dans les coins après la réussite, erreurs entourées = expériences mal prédites. |
| `Greenhouse.PlantPot` | Pot de plante : verrouillé avant la validation, commande d'arrosage (cran par cran, de 10 à 0), prédictions cohérentes avec la vraie serre sur au moins 90 % des 121 réglages (117 mesurés). |
| `Greenhouse.Calibration` | Mesures ci-dessous ; vérifie que le linéaire plafonne sous 80 % et que 4 à 6 neurones Tanh ou ReLU (taux 0,1) atteignent 95 % en moins de 40 epochs. |
| `Interaction.Interactor` | Dans un monde de test : rayon, distance maximale, focus et surbrillance, interaction, objet désactivé, source de visée de type contrôleur VR, interaction directe sans rayon. |
| `Lab.BuilderEditing` | Actions de configuration et leurs limites, verrouillage pendant l'entraînement, bouton lié à une couche absente puis présente. |
| `Lab.FloorButtonStartsTraining` | Un pawn qui entre dans la Box Collision du bouton au sol lance l'entraînement ; le bouton se relâche quand il repart. |
| `Lab.XORSuccessOpensDoor` | Bout en bout : `2 - 4 Tanh - 1 Sigmoid` réussit (57 epochs) et la porte s'ouvre complètement. |
| `Lab.ForgeAssembly` | Puzzle de la Forge : module vide ignoré, orbes = neurones, cristal = activation, emboîtement par proximité, types incompatibles refusés, prise en main d'une pièce emboîtée, verrouillage pendant l'entraînement. |
| `Lab.LinearModelKeepsDoorClosed` | Sans couche cachée : échec, porte fermée, conseil « Ajoutez une couche », configuration de nouveau modifiable. |

**Test de fumée du niveau réel** (`Smoke.LabPlaythrough`, contexte jeu uniquement) : il lance le jeu sur `L_Lab` et rejoue le parcours du joueur : briefing, module attrapé en le visant puis inséré dans la fente 1, 4 orbes et un cristal Tanh emboîtés, tour complet du levier, bouton au sol, validation du modèle, ouverture de la porte, sortie et écran de fin. Résultat actuel : modèle validé en 57 epochs, porte ouverte en 2,1 s.

```bat
"<UE_5.6>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<chemin>\ProjetNeuro.uproject" -game -nullrhi -nosound -ExecCmds="Automation RunTests NeuronForge.Smoke; Quit" -TestExit="Automation Test Queue Empty"
```

**Captures de contrôle visuel** (`Smoke.Screenshots`) : il capture les 4 pages du briefing, 6 vues du labo au repos (entrée, holotable, console, mode d'emploi, porte, serveurs), une vue pendant l'entraînement et l'écran de fin dans `Saved/Screenshots/NF_*.png`. Il demande un vrai rendu : remplacer `-nullrhi` par `-RenderOffscreen -ResX=1280 -ResY=720` dans la commande ci-dessus.

### Calibrage de la serre

Le niveau jouable utilise le problème de la serre : **160 expériences** (graine 2026), eau et lumière entre 0 et 10, la plante survit dans une zone ovale au milieu (centre eau 5 / lumière 5,5, rayons 3,3 et 3,1). Un bruit de mesure (écart-type 0,35) rend **7 expériences trompeuses** près de la frontière ; **30 %** des plantes survivent. Les entrées sont normalisées entre -1 et 1 pour le réseau.

Mesures du test `Greenhouse.Calibration` (même procédure d'entraînement que le jeu : SGD exemple par exemple, poids graine 42) :

| Architecture | Taux | 95 % atteint en | Meilleure précision |
|---|:---:|:---:|:---:|
| sans couche cachée | 0,5 | jamais | 70 % (répond toujours « meurt ») |
| 8 Linear / 8 Linear + 8 Linear | 0,1 | jamais | 70 % |
| 2 ou 3 Tanh | 0,5 | jamais | 82-83 % |
| 3 Tanh | 0,1 | 50 epochs (au-delà du maximum) | 96 % |
| **4 ReLU** | 0,1 | **10 epochs** | 98 % |
| **5 ReLU** | 0,1 | **16 epochs** | 99 % |
| **6 ReLU** | 0,1 | **11 epochs** | 99 % |
| **4 Tanh** | 0,1 | **33 epochs** | 96 % |
| **5 Tanh** | 0,1 | **23 epochs** | 98 % |
| **6 Tanh** | 0,1 | **23 epochs** | 98 % |
| 4 Tanh + 4 Tanh | 0,1 | 30 epochs | 99 % |
| 6 Sigmoid | 1,0 | 108 epochs | 95 % |
| 5 Tanh | 0,5 | jamais | 83 % (taux trop fort) |
| 5 Tanh | 2,0 | jamais | 89 % (instable) |
| 5 Tanh | 0,01 | 176 epochs | 98 % (taux trop faible) |

**Réglages retenus** (`FNFGreenhouseProfile`) : précision cible **95 %** (le modèle linéaire plafonne à 70 %, le bruit empêche d'atteindre 100 %), perte cible 0,3, **40 epochs au maximum**, **1,2 epoch par seconde**, taux d'apprentissage de départ **0,1**. Un bon modèle réussit donc en **8 à 28 secondes** de jeu ; un modèle qui échoue s'arrête au bout de 33 secondes. L'entraînement avance exemple par exemple, si bien que l'hologramme et la frontière de décision évoluent en continu.

**Mesures de calibrage XOR** (graine 42, arrêt à 100 % de précision et perte < 0,1) :

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
| 5 | Test de bout en bout et finitions | ✅ Terminé |
| 6 | Interface « pro » : briefing animé, objectifs en jeu, écran de fin | ✅ Terminé |
| 7 | Refonte visuelle du labo : ambiance sci-fi, éclairage réactif, animations | ✅ Terminé |
| 8 | Forge à pièces (puzzle physique) et grand hall de 45 × 30 m | ✅ Terminé |
| 9 | La serre : dataset, calibrage, holotable / Forge / écran d'état en mode serre | ✅ Terminé |
| 10 | Carnet du labo (frontière de décision en direct) | ✅ Terminé |
| 11 | Phase de test : le pot de plante | ✅ Terminé |
| 12 | Textes, tests et README pour la serre | ⏳ En cours |

---

## Roadmap

**MVP : niveau XOR jouable de bout en bout**
- Construction du réseau, entraînement visible, validation par la porte.

**Après le MVP**
- **Nouveaux niveaux et datasets** : cercles concentriques, spirales, classification multi-classes, chacun avec sa propre précision cible.
- **Visualisation de la frontière de décision** sur un écran du labo.
- **Optimiseurs supplémentaires** (Momentum, Adam) et choix de la fonction de perte.
- **Mode réalité virtuelle** : interactions à la main ou au contrôleur grâce à l'interface `INFInteractable` et au `UNFGrabberComponent` déjà prévus pour une main VR.
- **Interface UMG** pour le menu principal et les explications pédagogiques.
- **Retour pédagogique** : messages d'aide quand le modèle échoue (par exemple « Essayez d'ajouter une couche cachée »).
