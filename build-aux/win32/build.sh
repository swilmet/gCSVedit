#!/bin/sh

_pacman_root=/tmp/gcsvedit

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

create_pacman_root
