package fat

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"os"
)

const (
	sectorSize   = uint64(512)
	maxEntries   = 21
	entrySize    = 24
	nameSize     = 10
	metadataSize = 4
	lastSector   = uint32(math.MaxUint32)
	magic        = uint32(0x46415401)
	maxFileSize  = uint64(math.MaxUint16) * sectorSize
)

type entry struct {
	startingSector uint32
	sectorCount    uint16
	lastBytes      uint16
	present        bool
	reserved       uint8
	modeID         uint8
	typeID         uint8
	metadata       [metadataSize]byte
	name           [nameSize]byte
}

type fatSector struct {
	address uint32
	next    uint32
	entries [maxEntries]entry
}

type volume struct {
	file    *os.File
	path    string
	sectors uint32
	created bool
}

func openExisting(path string) (*volume, error) {
	f, err := os.OpenFile(path, os.O_RDWR, 0)
	return openVolume(f, path, err)
}

func openReadOnly(path string) (*volume, error) {
	f, err := os.Open(path)
	return openVolume(f, path, err)
}

func openVolume(f *os.File, path string, openErr error) (*volume, error) {
	if openErr != nil {
		return nil, fmt.Errorf("open target %s: %w", path, openErr)
	}
	v, err := inspectTarget(f, path)
	if err != nil {
		_ = f.Close()
		return nil, err
	}
	return v, nil
}

func inspectTarget(f *os.File, path string) (*volume, error) {
	info, err := f.Stat()
	if err != nil {
		return nil, fmt.Errorf("stat target %s: %w", path, err)
	}
	mode := info.Mode()
	isBlockDevice := mode&os.ModeDevice != 0 && mode&os.ModeCharDevice == 0
	if !mode.IsRegular() && !isBlockDevice {
		return nil, fmt.Errorf("target %s is neither a regular file nor a block device", path)
	}
	size := info.Size()
	if isBlockDevice {
		size, err = blockDeviceSize(f)
		if err != nil {
			return nil, fmt.Errorf("determine capacity of block device %s: %w", path, err)
		}
	}
	if size < int64(sectorSize) || size%int64(sectorSize) != 0 {
		return nil, fmt.Errorf("target %s size %d is not a positive multiple of %d bytes", path, size, sectorSize)
	}
	sectorCount := uint64(size) / sectorSize
	if sectorCount > uint64(lastSector) {
		return nil, fmt.Errorf("target %s has too many sectors: %d", path, sectorCount)
	}
	return &volume{file: f, path: path, sectors: uint32(sectorCount)}, nil
}

func createImage(path string, sizeMiB uint64) (*volume, error) {
	if sizeMiB == 0 {
		return nil, errors.New("--size must be greater than zero")
	}
	if sizeMiB > math.MaxInt64/(1024*1024) {
		return nil, errors.New("--size is too large")
	}
	bytes := sizeMiB * 1024 * 1024
	if bytes/sectorSize > uint64(lastSector) {
		return nil, errors.New("--size exceeds the FAT address space")
	}
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE|os.O_EXCL, 0644)
	if err != nil {
		return nil, fmt.Errorf("create target %s: %w", path, err)
	}
	if err := f.Truncate(int64(bytes)); err != nil {
		_ = f.Close()
		_ = os.Remove(path)
		return nil, fmt.Errorf("allocate target %s: %w", path, err)
	}
	return &volume{file: f, path: path, sectors: uint32(bytes / sectorSize), created: true}, nil
}

func (v *volume) close() error {
	return v.file.Close()
}

func (v *volume) readSector(address uint32) ([]byte, error) {
	if address >= v.sectors {
		return nil, fmt.Errorf("sector %d is outside target capacity", address)
	}
	data := make([]byte, sectorSize)
	n, err := v.file.ReadAt(data, int64(address)*int64(sectorSize))
	if err != nil && !errors.Is(err, io.EOF) {
		return nil, fmt.Errorf("read sector %d: %w", address, err)
	}
	if n != len(data) {
		return nil, fmt.Errorf("read sector %d: got %d of %d bytes", address, n, len(data))
	}
	return data, nil
}

func (v *volume) writeSector(address uint32, data []byte) error {
	if address >= v.sectors || len(data) != int(sectorSize) {
		return fmt.Errorf("invalid write of %d bytes to sector %d", len(data), address)
	}
	n, err := v.file.WriteAt(data, int64(address)*int64(sectorSize))
	if err != nil {
		return fmt.Errorf("write sector %d: %w", address, err)
	}
	if n != len(data) {
		return fmt.Errorf("write sector %d: wrote %d of %d bytes", address, n, len(data))
	}
	return nil
}

func (v *volume) sync() error {
	if err := v.file.Sync(); err != nil {
		return fmt.Errorf("sync target %s: %w", v.path, err)
	}
	return nil
}

func emptyFATSector() fatSector {
	s := fatSector{next: lastSector}
	for i := range s.entries {
		s.entries[i].startingSector = lastSector
	}
	return s
}

func decodeFATSector(address uint32, data []byte) (fatSector, error) {
	if len(data) != int(sectorSize) {
		return fatSector{}, fmt.Errorf("sector %d has invalid length %d", address, len(data))
	}
	if binary.LittleEndian.Uint32(data[0:4]) != magic {
		return fatSector{}, fmt.Errorf("sector %d has invalid FAT magic", address)
	}
	s := fatSector{address: address, next: binary.LittleEndian.Uint32(data[4:8])}
	for i := range s.entries {
		offset := 8 + i*entrySize
		flags := binary.LittleEndian.Uint16(data[offset+6 : offset+8])
		e := &s.entries[i]
		e.startingSector = binary.LittleEndian.Uint32(data[offset : offset+4])
		e.sectorCount = binary.LittleEndian.Uint16(data[offset+4 : offset+6])
		e.lastBytes = flags & 0x03ff
		e.present = flags&0x0400 != 0
		e.reserved = uint8(flags >> 11)
		e.modeID = data[offset+8]
		e.typeID = data[offset+9]
		copy(e.metadata[:], data[offset+10:offset+14])
		copy(e.name[:], data[offset+14:offset+24])
	}
	return s, nil
}

func encodeFATSector(s fatSector) []byte {
	data := make([]byte, sectorSize)
	binary.LittleEndian.PutUint32(data[0:4], magic)
	binary.LittleEndian.PutUint32(data[4:8], s.next)
	for i, e := range s.entries {
		offset := 8 + i*entrySize
		binary.LittleEndian.PutUint32(data[offset:offset+4], e.startingSector)
		binary.LittleEndian.PutUint16(data[offset+4:offset+6], e.sectorCount)
		flags := e.lastBytes | uint16(e.reserved&0x1f)<<11
		if e.present {
			flags |= 0x0400
		}
		binary.LittleEndian.PutUint16(data[offset+6:offset+8], flags)
		data[offset+8] = e.modeID
		data[offset+9] = e.typeID
		copy(data[offset+10:offset+14], e.metadata[:])
		copy(data[offset+14:offset+24], e.name[:])
	}
	return data
}

func (v *volume) writeEmptyRoot() error {
	root := emptyFATSector()
	root.address = 0
	return v.writeSector(0, encodeFATSector(root))
}

func (v *volume) scan() ([]fatSector, uint32, error) {
	var sectors []fatSector
	visited := make(map[uint32]bool)
	highWater := uint32(0)
	address := uint32(0)
	for {
		if address >= v.sectors || visited[address] {
			return nil, 0, fmt.Errorf("corrupt FAT chain at sector %d", address)
		}
		visited[address] = true
		data, err := v.readSector(address)
		if err != nil {
			return nil, 0, err
		}
		s, err := decodeFATSector(address, data)
		if err != nil {
			return nil, 0, err
		}
		if address > highWater {
			highWater = address
		}
		for i, e := range s.entries {
			if e.startingSector == lastSector {
				continue
			}
			if e.sectorCount == 0 {
				if e.startingSector != 0 || e.lastBytes != 0 {
					return nil, 0, fmt.Errorf("corrupt FAT entry %d in sector %d", i, address)
				}
				continue
			}
			if e.lastBytes == 0 || e.lastBytes > uint16(sectorSize) ||
				e.startingSector >= v.sectors || uint32(e.sectorCount) > v.sectors-e.startingSector {
				return nil, 0, fmt.Errorf("corrupt FAT entry %d in sector %d", i, address)
			}
			end := e.startingSector + uint32(e.sectorCount) - 1
			if end > highWater {
				highWater = end
			}
		}
		sectors = append(sectors, s)
		if s.next == lastSector {
			return sectors, highWater, nil
		}
		if s.next >= v.sectors || s.next == address {
			return nil, 0, fmt.Errorf("corrupt FAT link from sector %d to %d", address, s.next)
		}
		address = s.next
	}
}

func entrySizeBytes(e entry) uint64 {
	if e.sectorCount == 0 {
		return 0
	}
	return uint64(e.sectorCount-1)*sectorSize + uint64(e.lastBytes)
}

func copyBytesAt(dst io.WriterAt, dstOffset int64, src io.ReaderAt, srcOffset int64, size uint64) error {
	buffer := make([]byte, 128*1024)
	for size > 0 {
		count := uint64(len(buffer))
		if count > size {
			count = size
		}
		n, err := src.ReadAt(buffer[:count], srcOffset)
		if err != nil && !errors.Is(err, io.EOF) {
			return err
		}
		if n != int(count) {
			return io.ErrUnexpectedEOF
		}
		n, err = dst.WriteAt(buffer[:count], dstOffset)
		if err != nil {
			return err
		}
		if n != int(count) {
			return io.ErrShortWrite
		}
		srcOffset += int64(count)
		dstOffset += int64(count)
		size -= count
	}
	return nil
}
