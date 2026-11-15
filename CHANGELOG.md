# Changelog

## 0.1.0-alpha.40

- Share the full first native same-world victory XP; preserve unconscious/dead state.
- Evaluate shared reward eligibility by current faction path; keep the active native quest scope bounded to Cavalorn.
- Journal the first mutating Cavalorn choice, validate guest requests, match native reward deltas and notify the partner.
- Add complete mission/key permission pages, shared key opening and durable owner-only unique consumption inside verified quest contexts.
- Add explicit agreement on existing native save pairs and coordinated load/reconnect on fresh recovery timelines; no automatic saves or revival.
- Validate complete native/sidecar fingerprints, durable consent/restart records and committed-plan retry after one peer has already loaded.
- Protocol 13; both peers must update. Rollback requires matching backed-up state because new action semantics extend journal format 4.
- Preserve accepted clothing, artwork, renderer and comfort modules. Native acceptance remains pending.

## 0.1.0-alpha.39

- Capture direct native key unlock and wait for durable host confirmation before opening client inventory.
- End denied/cancelled chest interactions and quarantine late replies without duplicating transfers.
- Preserve native dialogue finishing work before returning NPCs to mirror AI; add bounded lifecycle diagnostics.
- Release held character forms before native load/disposal.
- Add bounded host-authoritative simple gate-switch and keyframe mover replication; protocol 12.
- Keep accepted body resources and save/journal formats unchanged. Native gameplay acceptance remains open.


## 0.1.0-alpha.38

- Restore native Friendly fire and Language row selection and logical navigation order.
- Validate forwarding targets and add five menu regression tests and bounded runtime diagnostics.
- Record user acceptance of female everyday clothing, retaining its resources unchanged.


## 0.1.0-alpha.37 — 2026-11-14

- Route direct native chest/corpse ACTION input through host-confirmed inventory recovery; block untracked client mutations and log the actual transfer path.
- Guard missing or cleared Sprint key bindings in the pinned compatibility output.
- Add optional opaque BC1 everyday clothing built locally from supplied art and the native female mesh, with hash/material diagnostics.
- Keep ambiguous creature matching conservative and report both nearest distances.
- 4,787 technical assertions pass; native duplication/clothing retests and system-crash investigation remain open. No game launched.

## 0.1.0-alpha.36 — 2026-11-14

- Fix immediate loot-lease timeout when journal callbacks publish newer timestamps than the frame clock; retain real expiry and add timestamp diagnostics.
- Allow authoritative Quickloot of mission keys/tablets; report input, target and blocking reasons.
- Stop unchanged character snapshots from flickering inventory readiness; retain pending-mutation guards.
- Keep captured NPC spawn anchors stable through culling and improve ambiguous-mapping diagnostics.
- Add optional pinned-renderer repair/rollback tools with backup and configuration preservation.
- 4,775 technical assertions pass. Container/recovery and host Quickloot need native retesting; the preceding shared route stopped in the tower.

## 0.1.0-alpha.35 — 2026-11-14

- Correct Ninja discovery: use OreboundCore.vdf, a matching Ninja/OreboundCore directory and uppercase native VDFS catalogue entries. Retain existing script symbols and migrate the previous archive after backup.
- Add 15 package discovery/catalogue regressions, including the previously ignored underscore name, wrong folder and mixed-case catalogue.
- Use one opaque BC1 menu mip. Native startup exposed unsupported small compressed mips despite successful offline decoding.
- Replace the separate pre-menu startup background with a stock-format RGB565 version of the same image; retain original logo textures.
- Keep the background consistent in out-of-game submenus and quit confirmation using per-menu texture state; preserve the in-game pause view.
- Preserve player configuration, saves and logs; 4,736 C++ assertions and 15 package regressions pass. Shared gameplay acceptance remains separate from the menu check.

## 0.1.0-alpha.34 — 2026-11-14

- Add an optional Union/Ninja main-menu artwork contract. Use the existing native full-screen window for the image, clear only the inner window background, and retain original Gothic II logos and menu items.
- Restore original background texture references before native menu teardown; no per-frame texture clearing, custom rendering callback or global texture override.
- Build a uniquely named opaque BC1 texture with independently verified mip colors and VDF contents. Validate the optional archive before use/install.
- Preserve in-game pause menus, native text/alpha, settings and saves. Add apply/restore/fallback diagnostics, optional source-only fallback and installer rollback checks.
- 4,735 technical assertions pass. Native Ninja parsing and visual selection/submenu checks remain open; no game launched.

## 0.1.0-alpha.33 — 2026-11-14

- Rename current product/source artifacts to Orebound Co-op / OreboundCoop; preserve legacy save and script identifiers.
- Add a complete English/German UI catalogue, per-PC Union menu language setting, live preview/persistence, native text encoding and safe fallback diagnostics.
- Extend the own Ninja contract to ABI 2 for local journal text; retain protocol 11 and existing persistence formats.
- Add safe legacy runtime/INI migration without duplicate DLLs, and English-first public documentation with a German companion.
- Validate 4,723 technical assertions on both Windows PCs and the independent public build, plus 20 content-identity checks. Cover mixed local language pairs, native encodings, migration/defaults and configuration idempotence. Native parser/menu/gameplay acceptance remains open.

## Previous engineering milestones

- Alpha.32: versioned Ninja interface and optional comfort module profiles; 3,671 technical assertions.
- Alpha.31: personal character/crime state, special movement, transformations and owned summons.
- Alpha.30: confirmed save pause, inventory readiness, item matching and bounded independent log rotation.
- Alpha.29: validated interrupted journal recovery and idempotent decisions.
- Alpha.28: confirmed partner collision correction; further inventory/save regression required.

Exact original German release records are retained in [the historical archive](docs/de/history/CHANGELOG-through-alpha32.md). They describe historical evidence, not current native acceptance.
