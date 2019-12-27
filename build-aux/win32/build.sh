#!/bin/sh

_version='0.7.0'
_pacman_root=/tmp/gcsvedit
_mingw_package_dir=mingw-w64-gcsvedit

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

install_gcsvedit_packages() {
	pushd "${_mingw_package_dir}" > /dev/null

	./build.sh
	pacman --upgrade mingw-w64-x86_64-gcsvedit-${_version}-1-any.pkg.tar.xz \
		--noconfirm --root "${_pacman_root}" || exit 1

	popd > /dev/null
}

create_pacman_root
install_gcsvedit_packages
