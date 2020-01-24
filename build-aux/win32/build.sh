#!/bin/sh

_version='0.7.0'
_pacman_root=/tmp/gcsvedit
_mingw_package_dir=mingw-w64-gcsvedit
_package_dir=/tmp/gcsvedit-package

# It’s like installing packages with pacman in a chroot.
create_pacman_root() {
	rm -rf "${_pacman_root}"
	mkdir -p "${_pacman_root}"

	pushd "${_pacman_root}" > /dev/null
	mkdir -p var/lib/pacman
	mkdir -p var/log
	mkdir -p tmp
	popd > /dev/null

	pacman -Syu --root "${_pacman_root}" || exit 1
	pacman -S filesystem bash pacman --noconfirm --root "${_pacman_root}" || exit 1
}

# Once the pacman root is created, we can install packages in it.
install_gcsvedit_packages() {
	pushd "${_mingw_package_dir}" > /dev/null

	./build.sh
	pacman --upgrade mingw-w64-x86_64-gcsvedit-${_version}-1-any.pkg.tar.zst \
		--noconfirm --root "${_pacman_root}" || exit 1

	popd > /dev/null
}

# Copy the result to create one directory where the application can be launched.
# That directory is relocatable, it can be moved elsewhere on the filesystem and
# it’s still possible to launch the application.
create_package_dir() {
	rm -rf "${_package_dir}"
	cp -r "${_pacman_root}/mingw64/" "${_package_dir}"
}

# Unfortunately the "package directory" contains way too much stuff, we need to
# remove things in order to make the size acceptable. Then it’s possible to take
# this directory to create an installer file, for example an *.msix file.
remove_useless_stuff_from_package_dir() {
	pushd "${_package_dir}" > /dev/null

	find . -name "*.a" -exec rm -f {} \;

	# Remove unneeded binaries
	find . -not -name "g*.exe" -name "*.exe" -exec rm -f {} \;
	rm -rf bin/gtk3-demo*.exe
	rm -rf bin/py*
	rm -rf bin/*-config

	# Remove other useless folders
	rm -rf var/
	rm -rf ssl/
	rm -rf include/
	rm -rf share/man/
	rm -rf share/readline/
	rm -rf share/info/
	rm -rf share/aclocal/
	rm -rf share/gettext*
	rm -rf share/terminfo/
	rm -rf share/tabset/
	rm -rf share/pkgconfig/
	rm -rf share/bash-completion/
	rm -rf share/gdb/

	# On windows we show the online help
	rm -rf share/gtk-doc/
	rm -rf share/doc/

	# Remove things from the lib/ folder
	rm -rf lib/terminfo/
	rm -rf lib/python2*
	rm -rf lib/pkgconfig/

	# Strip the binaries to reduce the size
	find . -name *.dll | xargs strip
	find . -name *.exe | xargs strip

	# Remove all translations, gCSVedit is in English only
	find share/locale/ -name "*.mo" | xargs rm
	find share/locale -type d | xargs rmdir -p --ignore-fail-on-non-empty

	popd > /dev/null
}

# Execute the different steps.
create_pacman_root
install_gcsvedit_packages
create_package_dir
remove_useless_stuff_from_package_dir
