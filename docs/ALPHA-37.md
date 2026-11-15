# Alpha.37: native inventory actions and optional avatar clothing

Protocol 11 and script ABI 2 are unchanged. This is a developer alpha; native two-PC acceptance remains open. No game was launched for this update.

The preceding tower test exposed a duplicated dagger: the guest received it while the host could still take it from the same chest. The native chest and corpse ACTION handlers can remove and grant items directly, bypassing `TransferItem`. Both direct input handlers are now intercepted before the mutation and routed through the existing host-confirmed recovery path. Untracked client source windows cannot authorize local inventory changes. Keyboard, mouse, partial-stack and take-all quantities are covered; closing remains available while a transfer is pending. Deposits retain their existing confirmed transfer path.

New `loot_ui_action`, `loot_transfer_blocked` and `recovery_transfer_request` events identify the native input path, item, quantity and transaction. Technical checks cover both native handler ABIs and the standalone input/quantity policy. The fix still needs an in-game take/put/corpse regression with known starting counts.

Merchant input retains the existing price/payment validation path; it is never converted into a free loot grant. Quantities are bounded before the native short-count interface.

The separately prepared Sprint compatibility archive now tolerates missing or cleared key bindings without calling Ikarus key decoders every frame. Its updated hash is pinned in the catalogue and plugin; bridge ABI 1 is retained. Other original archive entries and personal settings remain unchanged. Recreate the compatibility output from the original pinned archive rather than mixing old and new profiles between peers.

Optional female everyday clothing uses Union appearance selection plus an isolated resource archive, not a new equipped armor item. The local builder combines a supplied clothing atlas with a renamed copy of the user's native female body. Geometry, UVs and skeleton remain unchanged. Texture encoding is opaque BC1, 512×512, one mip; the withdrawn uncompressed multi-mip outfit is not reused. The original face and head remain unchanged. The archive is hash checked and actual model materials are logged. Without the optional matching archive, a source build retains the native avatar. Original game meshes and private artwork are not distributed in this source repository.

The ambiguous close-creature matching guard remains conservative; diagnostics now include both nearest distances. A separate system-crash investigation remains unresolved. Neither successful technical tests nor this update establishes that VM crashes are fixed.

Retest one-item conservation first: guest inventory +1 and shared chest −1, then reopen on both peers and ensure the host cannot take that item again. Continue with deposits, corpse actions, stack quantities, Quickloot, reconnect and save/recovery. Verify clothing from both views and open the controls menu with Sprint enabled. Do not classify a previously failed native test as passed from a static build alone.


Follow-up, 2026-11-15: the user accepted the female body texture. Its appearance is complete and the resource remains unchanged. The settings navigation fix is tracked separately in Alpha.38.
