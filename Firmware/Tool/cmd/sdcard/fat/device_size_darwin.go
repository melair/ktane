//go:build darwin

package fat

import (
	"fmt"
	"os"
	"syscall"
	"unsafe"
)

const (
	dkiocGetBlockSize  = uintptr(0x40046418)
	dkiocGetBlockCount = uintptr(0x40086419)
)

func blockDeviceSize(file *os.File) (int64, error) {
	var blockSize uint32
	if _, _, errno := syscall.Syscall(syscall.SYS_IOCTL, file.Fd(), dkiocGetBlockSize, uintptr(unsafe.Pointer(&blockSize))); errno != 0 {
		return 0, errno
	}
	var blockCount uint64
	if _, _, errno := syscall.Syscall(syscall.SYS_IOCTL, file.Fd(), dkiocGetBlockCount, uintptr(unsafe.Pointer(&blockCount))); errno != 0 {
		return 0, errno
	}
	if blockSize == 0 || blockCount > uint64(^uint64(0)>>1)/uint64(blockSize) {
		return 0, fmt.Errorf("invalid block geometry: %d blocks of %d bytes", blockCount, blockSize)
	}
	return int64(blockCount * uint64(blockSize)), nil
}
