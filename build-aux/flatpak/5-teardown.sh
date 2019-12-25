#!/bin/sh

flatpak uninstall --user com.github.swilmet.gcsvedit
flatpak remote-delete --user gcsvedit-repo
rm -rf gcsvedit/ repo/ .flatpak-builder/
