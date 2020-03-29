#!/bin/sh

rm -rf gcsvedit/ repo/
flatpak-builder gcsvedit com.github.swilmet.gcsvedit.yml || exit 1
flatpak build-export repo gcsvedit
