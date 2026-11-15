# Architecture

Union is the native main-thread core: Winsock UDP, session/sequence validation, reliable ordered events, bounded queues, world authority, local-player exclusion, actor lifecycle, inventory transactions and recovery. Movement snapshots remain replaceable. No engine access occurs on a network worker thread.

The host owns shared NPC/world decisions. Each PC owns its local character and UI language. Confirmed transaction IDs, commit/abort records and save-bound checkpoints prevent blind inventory replay. Journals remain format 4, recovery checkpoints format 2, personal-character archives format 1; protocol is 14.

The own Ninja layer provides versioned bounded script calls. ABI 2 supports probe, merchant preparation, corpse rune cleanup, confirmed local quest XP and local journal language. Native bindings and command state are restored after calls; unsupported/reentrant operations fail closed. See [Union/Ninja contract](UNION-NINJA.md).

English/German UI uses stable keys, local settings and shared build resources. Optional module activation requires exact supported evidence; the full content fingerprint still includes optional archives. Matching optional profiles are required on both PCs, while UI languages may differ.

Native addresses, API signatures, base scripts and assets are version-bound. Other engines, TCs and language-specific script baselines are not automatically compatible. Implementation does not establish native acceptance; see [test plan](TESTPLAN-CAVALORN.md).

Party rules use reliable bounded messages and main-thread native adapters. The active quest profile is Cavalorn; faction eligibility is evaluated at grant and does not imply general quest support. First decisions are durable per recovery timeline. Mission/key pages grant permissions without adding items, and a verified-context consumption debits the physical owner once. Existing-save consent records are checksummed; commit/reconnect phases can originate from either the old or already loaded fresh timeline, while initial prepared branches must be new. Native loads happen after menus unwind. The common load forks fresh recovery timelines, keeping later discarded transactions out of the restored world. See [Alpha.40](ALPHA-40.md).

Alpha.41 keeps durable loot completion behind a data-only disk worker; engine effects and confirmations run only after completion on the main thread. Equipment meshes are display-only and terminal NPC poses carry animation progress. Journal/checkpoint/character formats remain 4/2/1.
