#!/bin/sh

# Move file for use with mf, read more at https://github.com/greeenlaser/personal-stash/tree/main/mf

set -e

#
# References
#

EXTERNAL_DIR=external

KH_ORIGIN=../kalaheaders
KH_TARGET=${EXTERNAL_DIR}/kalaheaders

KC_VER=1-1-0
KC_ORIGIN=../kalacli/build/${KC_VER}
KC_TARGET=${EXTERNAL_DIR}/kalacli

#
# Copy dependencies
#

# Always a fresh start
rm -rf "${EXTERNAL_DIR}"
mkdir "${EXTERNAL_DIR}"

# KalaHeaders
mkdir "${KH_TARGET}"

mf --f "${KH_ORIGIN}/README.md" --t "${KH_TARGET}/README.md"
mf --f "${KH_ORIGIN}/LICENSE.md" --t "${KH_TARGET}/LICENSE.md"

mf --f "${KH_ORIGIN}/include" --t "${KH_TARGET}"

# KalaCLI
mkdir "${KC_TARGET}"

if [ -d "${KC_ORIGIN}/release-windows" ]; then
    mf --f "${KC_ORIGIN}/release-windows" --t "${KC_TARGET}/release-windows"
fi
if [ -d "${KC_ORIGIN}/release-windows-gnu" ]; then
    mf --f "${KC_ORIGIN}/release-windows-gnu" --t "${KC_TARGET}/release-windows-gnu"
fi
if [ -d "${KC_ORIGIN}/release-linux" ]; then
    mf --f "${KC_ORIGIN}/release-linux" --t "${KC_TARGET}/release-linux"
fi

if [ -d "${KC_ORIGIN}/debug-windows" ]; then
    mf --f "${KC_ORIGIN}/debug-windows" --t "${KC_TARGET}/debug-windows"
fi
if [ -d "${KC_ORIGIN}/debug-windows-gnu" ]; then
    mf --f "${KC_ORIGIN}/debug-windows-gnu" --t "${KC_TARGET}/debug-windows-gnu"
fi
if [ -d "${KC_ORIGIN}/debug-linux" ]; then
    mf --f "${KC_ORIGIN}/debug-linux" --t "${KC_TARGET}/debug-linux"
fi
