# NullpoMino source reference

The DC32 Tetris app is a native C adaptation of
[`nullpomino/nullpomino`](https://github.com/nullpomino/nullpomino) commit
`4de098dd0b48d991247313d8dba30b9721e6f9d9`.

The port uses the upstream piece geometry, Standard SRS wall kicks,
Bag-No-S/Z/O-first and Nintendo randomizers, `STANDARD`,
`STANDARD-FAST-B`, and `NINTENDO-R` rule behavior, and the Marathon,
Line Race, and Ultra mode timing/scoring tables. Java, netplay, replay,
AI, custom-rule editing, music, and upstream graphical assets are not
included.

`LICENSE` contains the current project BSD 3-Clause license.
`LICENSE-NULLNONAME` retains the BSD notice carried by the older engine,
piece, wall-kick, and mode sources adapted by this port.
