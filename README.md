# T4C Server Linux Migration Project (Final Step)

This project represents the successful 2026 port of a legacy C++ MMORPG server engine from Windows (Win32/MFC) to a modern Linux 64-bit environment.

## 🚀 Technical Achievements

- **Monolithic Linkage**: Transitioned from a DLL-based architecture to a single, high-performance Linux binary.
- **Symbol Isolation Protocol**: Implemented a regional prefixing system for NPCs and Monsters to resolve naming collisions (e.g., Arakas_Mage vs Stoneheim_Mage).
- **64-bit Modernization**: Comprehensive refactoring of the memory model and pointer alignment (uint64_t migration).
- **Native Build System**: Complete integration with CMake and GCC 14+.
- **Portability Layer**: Custom shim for MFC-specific types and Windows-centric API calls.

## Dossier `build/` (runtime)

Après `cmake` + `make`, lancer **depuis `build/`** :

| Fichier | Rôle |
|---------|------|
| `T4CServer` | Binaire (généré par CMake) |
| `T4CServer.ini` | **Obligatoire** — config (ODBC, ports, chemins maps/Lokalise). Non versionné. Copier `tools/runtime/T4CServer.ini.example` puis éditer. |
| `textfilter.ini` | Optionnel — filtre de mots |
| `*.log` | Créés au démarrage (`Debug.log`, `PCEdit.log`, …) |

`/etc/odbc.ini` : DSN `[T4C Server Authentication]` (voir `tools/odbc/t4c-dsn.example.ini`).

`./build.sh` **ne supprime plus** `T4CServer.ini` lors d’un rebuild (sauvegarde/restauration automatique).

## 🛠️ Project Structure

- `/GameOps`: Core game logic and NPC management.
- `/T4C Server`: Main server engine and networking layer.
- `/tools`: Custom automation scripts for the Linux build process.
- `/NPC <Region>`: Regional script definitions integrated into the main build.

## ⚖️ Legal & Credits

**Original Engine**: Developed by the original authors (c. 2010). All rights to the original T4C assets and lore belong to their respective owners.

**Linux Port & Refactoring (2026)**: 
- Lead Developer: Tom [tlloancy]
- AI Collaborator: Cursor/Gemini

*Disclaimer: This repository contains source code modifications intended for research, educational purposes, and software interoperability. No original game assets (maps, sounds, databases) are included.*
