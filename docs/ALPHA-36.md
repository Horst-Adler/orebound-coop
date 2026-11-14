# Alpha.36 — fixes from a supervised two-player tower test

A guest container transfer could disconnect both peers immediately with a 15-second loot-lease timeout. The frame clock was captured before incoming events; journal I/O then published a newer traffic timestamp. Unsigned subtraction interpreted this fresh traffic as a huge elapsed interval.

Elapsed time now saturates at zero when a callback sample is newer than the frame clock. Real expiry remains enforced. Lease diagnostics include both timestamps, actual age and outstanding application status. Related cached-clock comparisons use the same policy.

Additional findings addressed:

- Mission keys and stone tablets are no longer silently excluded from authoritative Quickloot merely because of the mission flag. Equipped/native-ineligible items and corpse armor remain protected.
- Quickloot input diagnostics report the configured key, focused object, player state and blocking reason. The exact previous host-side failure was not fully instrumented; native confirmation is still required.
- Periodic unchanged character snapshots/receipts no longer flicker readiness. Actual pending character mutations remain blocking.
- NPC identity anchors remain fixed after initial capture instead of drifting on subsequent culling. Ambiguous bindings are still refused and now carry better diagnostics.
- A separate optional renderer maintenance tool validates the pinned original GD3D11 archive, backs up changed files, preserves settings and supports manifest-based rollback. Renderer binaries remain separately obtained third-party software.

The preceding native test confirmed partner collision/movement, regular ground pickups on both peers, no observed duplication in a simultaneous pickup attempt, and independent saves retaining the connection. It stopped at the container failure; the Cavalorn route and quest acceptance were not reached.

Validation: 4,775 technical C++ assertions, 15 Ninja packaging regressions and 20 content identity checks passed. A private read-only test decoded copies of the interrupted real journals and verified idempotent simulated replay; this is not a native inventory/reload acceptance claim. No game was launched for this update. Protocol 11, script ABI 2 and existing persistence formats are retained.

Retest container open/take/put, host Quickloot, recovery after the interrupted transfer, and save/load before resuming the shared Cavalorn route. Preserve journals together with their corresponding saves.
