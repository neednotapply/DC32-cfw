# Vendored T-Rex Runner reference

This directory pins `wayou/t-rex-runner` commit
`5455bfa408ec6b707c7300ff194b7390733a766d` as the behavioral and visual
reference for the DC32 badge port.

- `index.js` is the upstream gameplay source.
- `assets/default_100_percent/100-offline-sprite.png` is the upstream LDPI
  sprite sheet used by the port.
- `LICENSE` is the upstream BSD 3-Clause license.

The badge implementation is a native C adaptation. The build validates the
sprite SHA-256 before generating its internal grayscale raster.
