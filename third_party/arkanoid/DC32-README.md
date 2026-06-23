# wkeeling/arkanoid source reference

The DC32 Arkanoid app is a native C adaptation of
[`wkeeling/arkanoid`](https://github.com/wkeeling/arkanoid) commit
`7e0e876cd034ebd62890e65352c7ef0b12b45df5`.

`tools/build_arkanoid_assets.py` downloads that pinned source archive, verifies
its SHA-256, scales its PNG graphics into the badge's centered 222x240 arena,
and links the generated RGB332 atlas into `arkanoid.DC32`.

The gameplay port retains the five upstream round layouts, scoring, brick
durability, enemy types, and power-up rules. The upstream project credits The
Spriters Resource for most game graphics, Positech Games for explosion
graphics, Geronimo and Pixel Sagas for fonts, and Taito Corporation for the
original 1986 game.
