#!/bin/sh

# not 100% sure
gpg_key=2C472470
gpg2 --export $gpg_key > gcsvedit.gpg

rm -rf gcsvedit/ repo/
flatpak-builder gcsvedit be.uclouvain.gcsvedit.json || exit 1
flatpak build-export --gpg-sign=$gpg_key repo gcsvedit
