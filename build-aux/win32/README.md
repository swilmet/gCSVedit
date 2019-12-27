To create the installer you need to follow the next steps:
- Install the latest version of [MSYS2](http://www.msys2.org/).
- Follow the MSYS2 instructions to update the packages with pacman.
- Install git: pacman -S git
- Clone this repository with git.
- `cd $to_this_directory`

Then, create the old installer (it worked for version 0.2):
- Edit make-gcsvedit-installer and set the right version of the application.
- ./make-gcsvedit-installer.bat

New attempt at packaging again gCSVedit, in 2019-2020, still with MSYS2:
- build.sh

The mingw package can be tested independently, see the README.md file in that
directory.
