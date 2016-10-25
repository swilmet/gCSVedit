#!/bin/sh

gpg_key=2C472470
gpg2 --export --armor $gpg_key > gcsvedit.gpg

rm -rf gcsvedit/ repo/
flatpak-builder gcsvedit be.uclouvain.gcsvedit.json || exit 1
flatpak build-export --gpg-sign=$gpg_key repo gcsvedit
