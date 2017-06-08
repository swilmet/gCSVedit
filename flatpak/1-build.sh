#!/bin/sh

rm -rf gcsvedit/ repo/
flatpak-builder gcsvedit be.uclouvain.gcsvedit.json || exit 1
flatpak build-export repo gcsvedit
