mingw-w64-gcsvedit readme
=========================

Installation
------------

With MSYS2.

- Copy a gCSVedit tarball in this directory.
- Adapt the PKGBUILD (the `pkgver` and `sha256sums` variables).
- Run build.sh.
- Adapt install.sh (the `version` variable) and run it.

Then, how to run the gCSVedit application
-----------------------------------------

- In the Windows File Explorer, go to the `C:\msys64\mingw64\bin\` directory
  and execute `gcsvedit.exe`.

- Or, in a Windows PowerShell:
```
> cd C:\msys64\mingw64\bin\
> .\gcsvedit.exe
```

In PowerShell it’s possible to see if there are warnings/errors printed in the
terminal, in case the application doesn’t work well.
