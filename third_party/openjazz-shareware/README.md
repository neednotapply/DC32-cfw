# Jazz Jackrabbit Shareware Data

`JAZZ.ZIP` is the original, unmodified Jazz Jackrabbit shareware archive
distributed by Epic MegaGames in 1994.

- Size: 1,420,245 bytes
- SHA-256: `385F685D804B239E2AC070A1C267824B4A6B7898072248646C939A03469D345E`
- Original timestamp: 1994-08-01 11:02:50

The archive's `LICENSE.DOC` permits online distribution and hardware bundling
when the shareware files remain intact, the software is provided without
charge, and Epic MegaGames is credited. The DC32 release copies this exact ZIP
to `/APPS/JAZZ.ZIP`. OpenJazz converts the required game files into
`/APPS/openjazz.pak` on the badge's first launch; the generated pack is not
redistributed.

Before the main menu appears, the badge builds and validates the complete
shareware graphics cache in the shared 3 MiB ROM staging window. The cache
contains the fonts, HUD, all three world tile atlases and palettes, composed
sprites (including `.018`), and bonus graphics. It is sealed read-only during
gameplay. DOOM and emulator ROM staging use the same window, so launching one
of them causes OpenJazz to rebuild its cache automatically on the next launch.

This DC32 build intentionally supports the bundled Turtle Terror shareware
data. Full-game packs are rejected with an explanatory message because their
optimized-cache contents are not yet covered.

Jazz Jackrabbit is copyright Epic MegaGames. OpenJazz is a separate GPL-licensed
engine and is not affiliated with or endorsed by Epic Games.
