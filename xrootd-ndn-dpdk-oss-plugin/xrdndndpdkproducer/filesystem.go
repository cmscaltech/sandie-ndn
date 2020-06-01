package xrdndndpdkproducer

/*
#include "filesystem-c-api.h"
*/
import "C"
import "strings"

// Filesystem types
const (
	posix  string = "posix"
	hadoop        = "hadoop"
	ceph          = "ceph"
)

// GetFilesystemType return filesystem type from C API
func GetFilesystemType(filesystemType string) C.FilesystemType {
	switch filesystemType = strings.ToLower(filesystemType); filesystemType {
	case hadoop:
		return C.HADOOP
	case ceph:
		return C.CEPH
	default:
		return C.POSIX
	}
}
