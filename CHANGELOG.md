# Changelog — T4C Server Linux (Final Step)

Historique des modifications serveur liées au chargement WDA, au boot et aux contournements de développement.

---

## 2026-05-18 — Boot WDA : traces, skips dev, seek créatures, fix `WorldMap`

### Contexte (problème initial)

- Au démarrage, le serveur affichait `Loading objects` puis **crashait** dans `WDAObjects::CreateFrom` (lecture de `csGmItemLocation`) avec une taille de chaîne aberrante (`0x6B6D6D6D` = motif ASCII « mmm »).
- **Cause racine format** : le compilateur Havoc/`wc` écrit le champ « Nombre de charges » en **4 octets** (`WriteLong`), alors que le serveur Linux lit `lCharges` en **8 octets** (`signed long` / LP64) → décalage cumulé sur tous les objets suivants.
- **Cause racine performance** : `WDAFile::Read` lit le fichier **octet par octet** (`fgetc` + déchiffrement XOR position-dépendant). Sur ~1658 objets (Worlds) + 227 (Edit), puis créatures, le boot peut prendre **plusieurs minutes** ou sembler bloqué sans logs.
- **Blocage placement au sol** : après chargement des définitions d’objets, `WDAInitObjects` place ~2601 objets via `create_world_unit` ; le premier spawn testé (`106` à 1601,2569, monde 0) ne terminait pas (boucle / mutex / `ViewFlag(BLOCKING)` aberrant).

### Données WDA attendues (hors dépôt serveur)

Les fichiers dans `build/WDA/` doivent être la variante **LP64** générée par le pipeline client `finalstep/client/second_approach/` (patch binaire + offsets cohérents), **pas** le Havoc brut réinstallé via `key_swaps/install_to_build.sh`.

| Fichier | MD5 LP64 attendu |
|---------|------------------|
| `T4C Worlds.WDA` | `f972edd2d4b663dfceb2127c41b0e1c0` |
| `T4C Edit.WDA` | `3f1dcf1a6d95066bdc39196f378c39fe` |

---

### Fichiers modifiés

#### `T4C Server/TFCInit.cpp`

**Traces `WDAInitObjects` (diagnostic uniquement — pas de nouvelle logique métier)**

- La boucle d’enregistrement des ~1885 définitions d’objets existait déjà ; ajout de logs `stderr` :
  - début : `registering N items…`
  - progression tous les **200** items : `item K / N (ID)`
  - fin : `WDAInitObjects complete`
- **But** : localiser un éventuel blocage pendant les formules / containers / boosts, distinct du chargement WDA ou des spawns au sol.

**Variable d’environnement `T4C_SKIP_GROUND_OBJECTS`**

- Si définie : **ne pas** exécuter la boucle qui appelle `WorldMap::create_world_unit` pour les positions au sol lues dans le WDA (~2601 entrées Worlds + Edit).
- Les **définitions** d’objets (stats, formules, etc.) restent chargées et enregistrées.
- Sinon : boucle inchangée + logs de progression (tous les 200 spawns) + trace détaillée sur les **3 premiers** spawns (`lookup`, `wID`, `create_world_unit…` / `done`).
- **Effet jeu** : pas d’objets physiques sur les cartes (coffres, clés au sol, etc.) tant que le skip est actif.

**Phase créatures dans `TFCInitMaps`**

- Log `[BOOT] WDA objects phase done, loading creatures…`
- Variable d’environnement **`T4C_SKIP_CREATURES`** :
  - appelle `WDACreatures::SkipSection` au lieu de `CreateFrom` ;
  - **ne pas** appeler `WDAInitCreatures` (aucun monstre enregistré depuis le WDA).
- Les sections **hives**, **area links**, **clans** continuent d’être chargées après le skip.

**Boot dev validé (2026-05-18)** avec :

```bash
export T4C_SKIP_GROUND_OBJECTS=1
export T4C_SKIP_CREATURES=1
./T4CServer
```

→ message `Server started on …` après ODBC / handler / packet manager.

---

#### `T4C Server/WDAObjects.cpp`

**Traces `CreateFrom` (diagnostic)**

- Avant la boucle objets : `loading N objects (fgetc decrypt — peut prendre 1–3 min)…`
- Tous les **100** objets : `object K / N, file pos=…`
- Après objets : `objects done`, nombre de **positions au sol**, `CreateFrom complete`

**But** : confirmer que le temps passé est bien dans la lecture WDA chiffrée, pas dans une autre phase.

---

#### `T4C Server/WDACreatures.cpp` et `WDACreatures.h`

**Nouvelle API `WDACreatures::SkipSection(WDAFile &, const std::string &wdaPath)`**

- Avance le curseur du fichier WDA de la fin de la section **objets + positions** jusqu’au début de la section **hives** (groupes de spawn / `creaturegroup` côté Havoc), **sans** construire les `CreatureData` en RAM.
- **Important** : le seek ne doit **pas** sauter les hives — seulement les créatures. Un premier implémentation seekait *après* les hives → `WDAHives::CreateFrom` lisait la section téléports → crash `Assertion ch != EOF` avec taille de chaîne ~1,8 Go.

**Table d’offsets (WDA LP64 `second_approach` uniquement)**

| Fichier | `startPos` (début créatures) | `endPos` (début hives) |
|---------|------------------------------|-------------------------|
| `T4C Worlds.WDA` | 24259466 | 24347015 (269 hives) |
| `T4C Edit.WDA` | 73203 | 94690 (87 hives) |

- Si `wdaFile.Tell() == startPos` → `WDAFile::Seek(endPos)` (instantané).
- Sinon → repli **`SkipCreaturesSlow`** : enchaînement de `Read()` identique à `CreateFrom`, sans stocker (lent).

**Variables `dwDummy`, `dblDummy`, `boDummy` dans `SkipOneCreature`**

- Ce ne sont **pas** des champs du format WDA ajoutés au projet.
- Variables **jetables** pour appeler `wdaFile.Read(…)` dans le **même ordre** que `WDACreatures::CreateFrom` et faire avancer le flux chiffré :
  - `dwBindedID`, `csID`, `csName`
  - 22× DWORD : stats (STR…LCK), résistances, pouvoirs, level/HP/dodge
  - 1× double : AC
  - 12× DWORD : apparence + équipement + aggro/clan/speed
  - 2× double : XP hit/death
  - 2× DWORD + bool : or min/max, `can_attack`
  - boucles : attaques (string + 5 DWORD), death flags (2 DWORD + bool), loot (DWORD + double)

**Note** : le log de `WDACreatures.cpp` ligne ~170 affiche encore `Loading objects.` — copier-coller historique ; la section lue est bien **creatures**.

---

#### `T4C Server/WDAFile.cpp` et `WDAFile.h`

- **`void Seek(long pos)`** : `fseek` sur le `FILE*` interne.
- Utilisé par `SkipSection` pour le saut rapide. Le déchiffrement XOR reste cohérent tant que les lectures reprennent à un offset exact de section.

---

#### `T4C Server/WorldMap.cpp`

**`WorldMap::SetBlockingUnit` — garde-fou hauteur de blocage**

- `uHeight = obj->ViewFlag(__FLAG_BLOCKING)` en `UINT` : si la valeur est **0** → forcé à **1**.
- Si `uHeight > where.Y + 1` → plafonné (évite une boucle `for (y = where.Y; y > where.Y - uHeight; y--)` avec `uHeight` énorme → quasi boucle infinie en unsigned).
- Lié au blocage observé lors du placement de l’objet au sol `106` ; à valider sans `T4C_SKIP_GROUND_OBJECTS`.

---

### Variables d’environnement (résumé)

| Variable | Effet |
|----------|--------|
| `T4C_SKIP_GROUND_OBJECTS` | Skip ~2601 `create_world_unit` pour objets WDA au sol ; définitions d’objets OK. |
| `T4C_SKIP_CREATURES` | Skip lecture + init créatures WDA ; seek vers hives ; hives/links/clans OK. |

---

### Limites connues (boot dev vs prod)

| Avec skips | Conséquence |
|------------|-------------|
| `T4C_SKIP_GROUND_OBJECTS` | Pas d’objets posés sur les cartes depuis le WDA. |
| `T4C_SKIP_CREATURES` | Pas de définitions de monstres WDA ; spawns hive possiblement vides ou incohérents. |
| Offsets `SkipSection` | Valides seulement pour les WDA LP64 listés ci-dessus ; autre build → repli lent ou désalignement. |

---

### Marche à suivre (prochaines étapes)

1. **Commit** ce lot serveur (traces + skips + `Seek` + `WorldMap`).
2. **Données** : déployer uniquement les WDA LP64 (`client/second_approach/install_to_build.sh` vers `build/WDA/`), vérifier les MD5.
3. **Fix structurel recommandé** : lire `lCharges` en `DWORD` (4 octets) dans `WDAObjects` → compatibilité Havoc brut sans regénérer tout le WDA LP64.
4. **Performance** : remplacer la lecture `fgetc` par blocs (`fread` + déchiffrement par buffer) dans `WDAFile::Read` — gain majeur sur objets/créatures.
5. **Placement au sol** : retester sans `T4C_SKIP_GROUND_OBJECTS` après fix `WorldMap` ; si blocage persiste, tracer `WorldMap::Lock` / `create_world_unit`.
6. **Créatures** : retester sans `T4C_SKIP_CREATURES` après fix perf ou fix `lCharges`.
7. **Client** : test connexion + entrée en jeu contre ce serveur.

### Correctifs hors dépôt serveur (référence)

| Emplacement | Rôle |
|-------------|------|
| `finalstep/client/second_approach/` | Patch LP64, `wc` adapté, `patch_wda_lp64.py`, vérif Python |
| `finalstep/client/key_swaps/` | Clés XOR 1.61 / 1.68 — **ne pas** réinstaller Havoc brut sur `build/WDA/` après LP64 |

---

## Modèle pour les entrées futures

```markdown
## YYYY-MM-DD — Titre court

### Contexte
…

### Fichiers modifiés
#### `T4C Server/…`
…

### Variables d'environnement
…
```
