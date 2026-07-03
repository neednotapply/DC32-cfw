# YSoccer 19 reference

The DC32 Sensible Soccer app is a native C adaptation of YSoccer 19, pinned
to SourceForge commit `689d8b5cb162e5f270a4d6df44b5eeb82b6170d7`.

- Upstream: https://sourceforge.net/p/ysoccer/code/
- License: GNU General Public License version 2.0.
- `upstream/` contains the complete pinned Java core and GPL runtime assets
  used by the badge build; `reference/` retains the small review-oriented
  source subset from the initial implementation.
- The pack builder presents 24 recognizable national teams across four compact
  kit styles, using source-derived player records. Match graphics preload into
  the shared XIP cache at app launch, leaving only team setup at match start.
  The pack contains the required sprite atlases, origins, stadium boundaries,
  goals, shadows, and balls.
- YSoccer's 1996-97 export index refers to original SWOS team files that are
  not distributed upstream. Users may supply those files through import.
- No original Sensible Soccer or Sensible World of Soccer executable, ROM, or
  proprietary data file is included.

The complete GPL-2.0 text is available in `COPYING` in this directory.
