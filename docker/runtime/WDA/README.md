# WDA runtime (LP64)

Place ici les trois fichiers **obligatoires** (variante LP64, pas Havoc Win32 brut) :

- `T4C Worlds.WDA`
- `T4C Edit.WDA`
- `NPCs.WDA`

MD5 de référence (CHANGELOG) :

| Fichier | MD5 |
|---------|-----|
| `T4C Worlds.WDA` | `f972edd2d4b663dfceb2127c41b0e1c0` |
| `T4C Edit.WDA` | `3f1dcf1a6d95066bdc39196f378c39fe` |

Remplir ce dossier depuis un `build/WDA/` existant :

```bash
./docker/sync-runtime-from-build.sh
```

Ou monter un autre chemin via `T4C_RUNTIME_ASSETS` dans `docker compose` (voir `docker/README.md`).
