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

  if [[ $PKGNAME == *"consumer"* ]]; then
    echo '#cgo CFLAGS: '$CFLAGS' '$DPDKFLAGS
  elif [[ $PKGNAME == *"producer"* ]]; then
    echo '#cgo CFLAGS: '$CFLAGS' '$DPDKFLAGS
  elif [[ $PKGNAME == *"filesystem"* ]]; then
    echo '#cgo CXXFLAGS: '$CXXFLAGS' '
  fi

  echo -n '#cgo LDFLAGS:'
  if [[ $PKGNAME == *"consumer"* ]]; then
    echo $LDFLAGS' '$DPDKLDFLAGS
  elif [[ $PKGNAME == *"producer"* ]]; then
    echo $LDFLAGS' '$DPDKLDFLAGS' '$LIB_FILESYSTEM
  elif [[ $PKGNAME == *"filesystem"* ]]; then
    echo $LIB_FILESYSTEM
  fi

  echo '*/'
  echo 'import "C"'
) | gofmt -s > $PKG/cgoflags.go
