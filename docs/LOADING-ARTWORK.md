# World loading artwork — Alpha.41 artwork revision 2

This optional resource revision uses Gothic II's existing loading-picture
selection. It introduces no world-transition hook, drawing loop or new gameplay
feature. Protocol 14, Ninja script ABI 2, menu ABI 1 and persistence formats are
unchanged. The matching Union binary is rebuilt with the new artwork hash;
both peers need the same resulting content fingerprint.

| Resource | World | Artwork |
| --- | --- | --- |
| `LOADING-C.TEX` | Generic fallback | Existing main illustration |
| `LOADING_NEWWORLD-C.TEX` | Khorinis | Existing main illustration |
| `LOADING_OLDWORLD-C.TEX` | Valley of Mines | Approved Minental V2 |
| `LOADING_ADDONWORLD-C.TEX` | Jharkendar | Approved Jharkendar V5 |
| `LOADING_DRAGONISLAND-C.TEX` | Irdorath | Approved Irdorath V2 |

The own menu texture, startup texture, generic loading texture and Khorinis
loading texture remain byte-identical to the preceding artwork package. Original
logos, inner menu transparency, progress bars and avatar clothing are untouched.

All loading textures retain the stock-compatible RGB565 layout, 1024×1024,
one mip. Source PNGs remain unchanged. The manifest records each world, source
hash and texture hash; the three regional textures must differ from one another
and from the generic fallback.

## Build inputs

The source repository includes the converter and hash catalogue, not the
artwork PNGs, game resources or a public binary release. A private development
checkout can supply the approved three images in `assets/loading-screens/approved`,
or use `--loading-artwork-dir` with `Build-MenuArtwork.py`. The expected approved
filenames and hashes are explicitly pinned in the converter.

When the default approved directory is absent, the existing generic artwork
build remains possible. If the directory exists, all three pinned files are
required: an incomplete or altered set fails rather than silently selecting
another illustration. Rebuild the Union DLL after conversion updates its
generated archive hash header.

## Validation and scope

The private verification compares all converted pixels to independently packed
RGB565 source samples, checks both archive mount orders against installed stock
texture archives, and rejects missing/altered approved assets. Existing native
resource and technical regression checks remain separate from visual acceptance.

Host, portable client and actual guest installation are compared using the
42-file content profile, with runtime backups and preservation checks for saves,
journals, personal configuration and renderer files. No game is launched for
these checks. In-game loading-screen acceptance remains open.

Providing regional illustrations does **not** implement or accept coordinated
co-op world/chapter transitions; those remain Beta work.
