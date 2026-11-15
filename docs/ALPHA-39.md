# Alpha.39: native interactions and shared gates

- Capture direct key-based unlocks in the native lock interaction path as well as standalone Unlock. Nested paths produce one durable decision. Client inventory waits for host confirmation; denial, timeout and Escape release an interaction even when no inventory opened. Targeted mob/use messages are removed; unrelated event queues are retained. Cancelled late replies cannot reopen an inventory or invent item grants.
- Track requested, active and finishing dialogue before returning NPCs to host mirroring. Preserve local end-state AI and queued work, bound abandoned cleanup, and arm the lease at native ProcessInfos. Add phase and watchdog diagnostics for repeated conversations.
- Release held hero/transformation references before native world loading and disposal, rather than after loading. This corrects a concrete ownership error associated with the observed one-reference reload warning; in-game confirmation is still required.
- Synchronize basic script-free target switches attached directly to named native two-state keyframe movers. Validate actor readiness, distance, occupation and identity on the host. Replicate state and movement, suppress trigger echoes and stale requests, and resend baseline state after reconnect. Unnamed buttons use their target plus anchor. Native movement direction is preserved for save/disconnect continuity.

The examined tower fireplace button targets `EVT_XARDAS_SECRET_01` directly. Scripted workstations, arbitrary quest triggers, teleporters, timed movers and multi-state mechanisms are outside this adapter. This is not general quest/world-script synchronization.

The menu artwork guard also consults the native running-session state, preventing the startup background from obscuring a loaded pause menu when the original menu flag is unset. Main-menu diagnostics now distinguish the running world from presentation flags.

**Protocol 12** requires both peers to update. Ninja script ABI 2, menu artwork ABI 1 and save/journal formats are unchanged. Existing accepted female clothing and artwork resources remain unchanged. Union owns these changes; no additional Ninja script hook is needed.

Validation includes codec truncation/bounds, reliable UDP transport, denied/stale/duplicate states, unlock-confirmation ordering, cancelled interactions and dialogue handoff policies, plus native ABI and resource checks and an independent public build. Technical checks do not run Gothic. Native acceptance must repeat conversations, key-chest open/cancel, gate operation from each peer and reconnect. A previously accepted normal chest withdrawal remains recorded; deposits, corpses and campaign acceptance remain separate.

Technical closeout: 4,955 C++ assertions passed on host, actual client and independent public source build; eleven native/resource/configuration check groups and five menu regressions passed. Three installations have the same 42 content files; all 1,692 renderer files per installation are retained. Native gameplay remains pending and no game was launched.
