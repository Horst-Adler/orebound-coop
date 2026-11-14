# Alpha.35 — Ninja discovery and menu texture correction

The host starts, but Alpha.34 logs showed `Ninja menu contract unavailable`. The own archive was readable by ZenKit yet ignored by native Ninja/VDFS: its basename contained an underscore, its internal patch directory did not match that basename, and its catalogue used mixed-case names.

The archive is now `OreboundCore.vdf`, with `Ninja/OreboundCore` and uppercase native catalogue entries. Existing Daedalus symbol names remain compatible. The installer backs up/removes the previous own archive and updates both peers' content IDs. These detection requirements are now checked during builds, with 15 dedicated regressions. See the [official Ninja package naming rules](https://github.com/szapp/Ninja/wiki/Solution#vdf-header).

Native verification then recorded `menu_artwork_apply` and exposed another issue: small DXT1 mips were rejected by Gothic. The menu artwork now has one opaque mip at 2048x1024, matching the single-mip approach used by original menu art. The block dimensions are explicitly checked. An independent decoder alone was insufficient to prove native compatibility.

4,736 existing technical assertions still pass. Menu verification does not establish co-op gameplay acceptance; the shared Cavalorn route remains necessary. No save is loaded for the bounded startup check.

The artwork also applies to out-of-game submenus and quit confirmation. Each native menu retains/restores its own background references independently; nested menus do not overwrite the parent state. The in-game pause menu remains unchanged.

The separate pre-menu InitScreen uses STARTSCREEN.TGA. A matching stock-format RGB565 override now displays the same artwork during that startup transition, while original logo textures remain unchanged.

Host display acceptance: the user confirmed the main menu, transparent windows, original logos, out-of-game submenus and startup transition. Native logs confirm apply/restore without the previous DXT texture-buffer error. No save was loaded for the checks; shared gameplay acceptance remains open.
