# Alpha.41 – replica fixes and loading backgrounds

Protocol **14**, Ninja script ABI **2**, menu artwork ABI **1**. Both peers must
update together. No game was launched by the agent; native two-PC acceptance of
these changes remains pending.

## Changes

- Discover switches and movers through the complete world tree. The native
  `oCWorld::voblist` excludes `zCMover`; resolve only unique targets and log the
  resulting registry. Retain host authority and reconnect baselines.
- Prepare an explicit guest talk target before native queued-item checks can
  reject it. Release only that peaceful mirrored NPC from stranded routine work;
  retain dialogue completion/timeouts and log body/AI/item/message state.
- Separate dead/unconscious replicas from living locomotion. Replicate animation
  progress, hold terminal poses, and restore original model timing at handoff.
  No automatic revival and no replay of death movement on each frame.
- Replicate equipped melee/ranged instances and draw/holster state. Attach only
  display meshes; temporary visual objects never enter native inventories,
  equipment slots or the world, preventing additional lootable weapons.
- Move durable writes for hot quickloot/withdrawal/ground-pickup paths to a data-only
  file worker. Poll completion on the main thread before applying effects or
  sending receipts. Preserve flush/write-through durability, ordered decisions,
  source ledger revisions and replay. Other synchronous special paths remain
  protected. In-game frame-time acceptance is still required.
- Include native insertion waypoints in NPC matching. Distant or ambiguous
  existing candidates must not produce a false removal tombstone. Genuine
  identity ties remain unresolved and logged rather than guessed.
- Guard saving while an inventory decision is still pending. Settle an already
  durable completion without blocking before save readiness is evaluated.
- Buffer diagnostic output during normal logging, with approximately 250 ms
  flushing and immediate flushing for errors/test markers. Explicit shutdown and
  export flush as well. Diagnostic buffering does not weaken journal durability.

The matching separately supplied own artwork archive now includes the generic
loading background and the NewWorld, OldWorld, AddonWorld and DragonIsland variants.
They use the existing approved image and native RGB565 texture layout. Progress
bars and original logos are retained. Providing world loading pictures does not
implement coordinated world transitions.

## Scope

The previous test confirmed repeated Xardas conversations, ordinary chest/key
interaction, working pickup/quickloot and basic full shared combat XP. Those
results are retained. The gate, guest Cavalorn handoff, terminal animation,
partner equipment and pickup performance need targeted testing in this build.
The bandit quest and its reward/persistence cases have not been accepted.

Sleep/world time, general shared pause, coordinated world/chapter transitions,
automatic creation of paired saves and general companion handling are **Beta**
work. None is implemented by this update. The existing Alpha.40 consent-based
linking/loading of already written saves remains available for acceptance.

Journal 4, checkpoint 2 and character archive 1 are retained. Update tools preserve
personal options, saves, journals and logs, and back up runtime files before
installation. Original game data, third-party archives, private tests/logs and
separately supplied artwork are not distributed in this repository.

## Validation

11,562 technical assertions and thirteen native/resource check groups pass in the
local build. Tests cover actual asynchronous disk completion/failure, durable
journal restart, equipment/animation codecs and conservative NPC absence checks.
Independent source/client installation verification is tracked separately; this
document is not a claim of completed native gameplay or a public binary release.

## Installation verification

The host, portable copy and actual guest installation have matching 42-file content profiles. The DLL and optional own artwork archive are the only changed runtime contents relative to Alpha.40. All 1,692 renderer files per installation were retained; 191 host and 148 guest save/state/log files were preserved. The host, headless guest and independent public source build pass 11,562 technical assertions. The host also passed interactive keyboard checks. 81 own source files were compared with the implementation used for deployment. Native gameplay acceptance remains pending; no game was started.
