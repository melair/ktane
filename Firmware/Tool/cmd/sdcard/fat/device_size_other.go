//go:build !darwin

package fat

import (
	"io"
	"os"
)

func blockDeviceSize(file *os.File) (int64, error) {
	return file.Seek(0, io.SeekEnd)
}
