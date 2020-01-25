To create the `*.msix` file you need to follow the next steps:
- Install the latest version of [MSYS2](http://www.msys2.org/).
- Follow the MSYS2 instructions to update the packages with pacman.
- Install git: pacman -S git
- Clone this repository with git.
- `cd $to_this_directory`
- Run build.sh

Then in PowerShell run as administrator:

```
cd 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\'
.\makeappx.exe pack /d "C:\msys64\tmp\gcsvedit-package\" /p "C:\gCSVedit-0.7.0.0.msix"
```

The mingw package can be tested independently, see the README.md file in that
directory.
