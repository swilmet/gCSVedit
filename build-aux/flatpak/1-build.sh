#!/bin/sh

rm -rf gcsvedit/ repo/
flatpak-builder gcsvedit com.github.swilmet.gcsvedit.json || exit 1
flatpak build-export repo gcsvedit
