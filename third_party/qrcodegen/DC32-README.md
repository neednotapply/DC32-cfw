# QR Code Generator source reference

The Laser Tag sync screen uses the zero-allocation C implementation from:

- Repository: https://github.com/nayuki/QR-Code-generator
- Branch: `master`
- Commit: `2c9044de6b049ca25cb3cd1649ed7e27aa055138`
- Source paths: `c/qrcodegen.c`, `c/qrcodegen.h`
- License: MIT; see `LICENSE` in this directory.

The library is used without protocol changes and is constrained to QR Model 2
versions 1 through 15 for the badge display and fixed workspace budget.
