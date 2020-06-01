#!/bin/bash
set -e

if [[ $# -ne 1 ]]; then
  echo 'USAGE: mk/cgoflags.sh package...' >/dev/stderr
  exit 1
fi

PKG=$(realpath --relative-to=. $1)
PKGNAME=$(basename $PKG)

source mk/cflags.sh

(
  echo 'package '$PKGNAME
  echo
  echo '/*'
  echo '#cgo CFLAGS: '$CFLAGS
  echo -n '#cgo LDFLAGS:'
  echo -n ' -L'$NDNDPDKLIBS_PATH
  echo -n ' -L'$LOCALLIBS_PATH
  echo -n ' '$NDNDPDKLIBS
  echo -n ' '$LIB_COMMON
  if [[ $PKGNAME == *"producer"* ]]; then
    echo -n ' '$LIB_FILESYSTEM
  fi
  echo ' '$LIBS
  echo '*/'
  echo 'import "C"'
) | gofmt -s > $PKG/cgoflags.go
