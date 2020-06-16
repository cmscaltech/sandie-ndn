package xrdndndpdkfilesystem

/*
#include "filesystem-c-api.h"
*/
import "C"
import (
	"fmt"
	"strings"
	"unsafe"
)

// Filesystem types
const (
	posix  string = "posix"
	hdfs          = "hdfs"
	cephfs        = "cephfs"
)

// GetFilesystemType return filesystem type from C API
func GetFilesystemType(_type string) C.FilesystemType {
	switch _type = strings.ToLower(_type); _type {
	case hdfs:
		return C.FT_HDFS
	case cephfs:
		return C.FT_CEPHFS
	default:
		return C.FT_POSIX
	}
}

// GetFilesystem return filesystem object
func GetFilesystem(_type string) (fileSystem unsafe.Pointer, e error) {
	fileSystem = C.libfs_newFilesystem(GetFilesystemType(_type))
	if nil == fileSystem {
		return nil, fmt.Errorf("Unable to get filesystem object")
	}

	return fileSystem, nil
}

// FreeFilesystem object
func FreeFilesystem(fileSystem unsafe.Pointer) {
	C.libfs_destroyFilesystem(fileSystem)
}
