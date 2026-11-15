# Optional main-menu artwork — Alpha.35

The intended layout is an own full-screen combat image with **Orebound Co-op** centered at the top, a transparent inner main-menu background, and the original Gothic II / Night of the Raven logos and native menu entries in front. The out-of-game main menu, settings/load menus and quit confirmation share the artwork and transparent inner background. In-game pause menus retain the game view.

Ninja adds two uniquely named constants to its menu parser via `Ninja/OreboundCore/Menu_G2.src`. Union validates this menu ABI 1 contract and the matching optional archive, then uses the already existing full-screen native `m_pWindow`. It removes only `m_pInnerWindow`'s background texture. It does not rewrite `MENU_MAIN`, its items, original logos, alpha, geometry, renderer state or per-frame callbacks. Original texture references are retained per native menu and restored before its ScreenDone, including nested menus. Content ABI remains 2; protocol remains 11.

The archive `Data/OreboundCoop_Menu.vdf` contains the uniquely named `OREBOUND_COOP_MENU-C.TEX` for menus and a deliberate `STARTSCREEN-C.TEX` override for the startup transition before menus exist. The startup entry matches the stock single-mip RGB565 format at 1024x1024; menus use opaque single-mip BC1. Colors and volume entries are independently read back. Original logo textures remain untouched.

## Build and install

This source repository contains the conversion tool and expected hash catalogue, not an artwork binary or original game resources. The development artwork is generated locally. To build with your own suitable source image, install Pillow and zenkit and provide it explicitly:

```powershell
.\scripts\Build-Alpha.ps1 -SdkRoot .\local\sdk\GothicCoop -MenuArtworkSource .\local\my-background.png
.\scripts\Install-Orebound.ps1 -GamePath 'C:\Games\Gothic II' -MenuArtworkDirectory .\build\alpha\menu
```

The conversion updates the own texture hash header and `config/menu-artwork.json`; compile the DLL afterwards and use the same resulting files on both PCs. A source-only build without the optional archive keeps the original menu. A supplied or retained artwork archive must match the compiled catalogue. The installer backs up the optional archive alongside owned runtime files and rolls it back on failed peer validation.

Set `[Interface] MenuArtwork=0` in the plugin INI while Gothic is closed to keep native menu styling. The early startup image remains while the optional archive is installed; use the runtime rollback manifest to restore the complete previous artwork state. Existing language/player options are retained. New diagnostics: `menu_artwork_apply`, `menu_artwork_restore`, `menu_artwork_fallback`.

## Native acceptance still required

Check warm colors, one full-screen picture, centered title, original foreground logos, readable selection, repeated Up/Down changes, settings/load/back navigation, and regular game exit. After loading a game, verify the pause menu uses the game view. Do not infer a passed visual test from build, pixel decoding or native ABI checks. Host startup is checked without loading a save.

Ninja's menu SRC format and independent menu parser are documented in the [official injection guide](https://github.com/szapp/Ninja/wiki/Inject-Changes#daedalus-scripts). No foreign menu script or Ikarus/LeGo implementation is copied into this feature.

Archive discovery is now validated: the alphanumeric basename `OreboundCore` matches `Ninja/OreboundCore`, and on-disk catalogue names are uppercase for native VDFS lookup. Compressed menu textures use one mip with dimensions divisible by four.

Host display acceptance: the user confirmed the main menu, transparent windows, original logos, out-of-game submenus and startup transition. Native logs confirm apply/restore without the previous DXT texture-buffer error. No save was loaded for the checks; shared gameplay acceptance remains open.

Alpha.41 adds native RGB565 replacements for `LOADING`, `LOADING_NEWWORLD`, `LOADING_OLDWORLD`, `LOADING_ADDONWORLD` and `LOADING_DRAGONISLAND` to the same optional matching artwork archive. The approved image, progress bars, original logos and native menu geometry are retained. Loading-picture acceptance is separate from coordinated world-transition support, which is Beta work.
