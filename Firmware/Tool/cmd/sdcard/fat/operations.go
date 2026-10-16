package fat

import (
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"strings"
	"text/tabwriter"
)

type entryLocation struct {
	sectorIndex int
	entryIndex  int
}

func runAdd(targetPath, sourcePath string, key fileKey, metadata [metadataSize]byte, out io.Writer) (err error) {
	source, err := os.Open(sourcePath)
	if err != nil {
		return fmt.Errorf("open source %s: %w", sourcePath, err)
	}
	defer source.Close()
	sourceInfo, err := source.Stat()
	if err != nil {
		return fmt.Errorf("stat source %s: %w", sourcePath, err)
	}
	if !sourceInfo.Mode().IsRegular() {
		return fmt.Errorf("source %s is not a regular file", sourcePath)
	}
	if sourceInfo.Size() < 0 || uint64(sourceInfo.Size()) > maxFileSize {
		return fmt.Errorf("source size %d exceeds maximum %d", sourceInfo.Size(), maxFileSize)
	}

	v, err := openExisting(targetPath)
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := v.close(); err == nil && closeErr != nil {
			err = fmt.Errorf("close target %s: %w", targetPath, closeErr)
		}
	}()
	targetInfo, err := v.file.Stat()
	if err == nil && os.SameFile(sourceInfo, targetInfo) {
		return errors.New("source and target must be different files")
	}

	sectors, highWater, err := v.scan()
	if err != nil {
		return err
	}
	match := entryLocation{-1, -1}
	free := entryLocation{-1, -1}
	for si := range sectors {
		for ei, e := range sectors[si].entries {
			if e.startingSector == lastSector {
				if free.sectorIndex < 0 {
					free = entryLocation{si, ei}
				}
				continue
			}
			if match.sectorIndex < 0 && keysEqual(e, key) {
				match = entryLocation{si, ei}
			}
		}
	}

	size := uint64(sourceInfo.Size())
	required := uint32((size + sectorSize - 1) / sectorSize)
	appendSector := uint64(highWater) + 1
	needsFATSector := match.sectorIndex < 0 && free.sectorIndex < 0
	dataStart := appendSector
	if needsFATSector {
		dataStart++
	}
	if match.sectorIndex >= 0 {
		old := sectors[match.sectorIndex].entries[match.entryIndex]
		atTail := old.sectorCount != 0 && old.startingSector+uint32(old.sectorCount)-1 == highWater
		if required <= uint32(old.sectorCount) || atTail {
			dataStart = uint64(old.startingSector)
		}
	}
	if required == 0 {
		dataStart = 0
	}
	if needsFATSector && (appendSector >= uint64(v.sectors) || appendSector >= uint64(lastSector)) {
		return errors.New("no space for another FAT sector")
	}
	if required != 0 && (dataStart >= uint64(lastSector) || dataStart+uint64(required) > uint64(v.sectors)) {
		return fmt.Errorf("not enough space for %d-byte source", size)
	}

	if size != 0 {
		if err := copyBytesAt(v.file, int64(dataStart*sectorSize), source, 0, size); err != nil {
			return fmt.Errorf("write payload: %w", err)
		}
	}
	newEntry := entry{
		startingSector: uint32(dataStart),
		sectorCount:    uint16(required),
		present:        true,
		modeID:         key.modeID,
		typeID:         key.typeID,
		metadata:       metadata,
		name:           key.name,
	}
	if required != 0 {
		newEntry.lastBytes = uint16((size-1)%sectorSize + 1)
	}

	if needsFATSector {
		newSector := emptyFATSector()
		newSector.address = uint32(appendSector)
		newSector.entries[0] = newEntry
		if err := v.writeSector(newSector.address, encodeFATSector(newSector)); err != nil {
			return err
		}
		tail := &sectors[len(sectors)-1]
		tail.next = newSector.address
		if err := v.writeSector(tail.address, encodeFATSector(*tail)); err != nil {
			return err
		}
	} else {
		location := free
		if match.sectorIndex >= 0 {
			location = match
		}
		sectors[location.sectorIndex].entries[location.entryIndex] = newEntry
		s := sectors[location.sectorIndex]
		if err := v.writeSector(s.address, encodeFATSector(s)); err != nil {
			return err
		}
	}
	if err := v.sync(); err != nil {
		return err
	}
	_, err = fmt.Fprintf(out, "added %s to %s: %d bytes\n", keyDisplay(key), targetPath, size)
	return err
}

func runRemove(targetPath string, key fileKey, out io.Writer) (err error) {
	v, err := openExisting(targetPath)
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := v.close(); err == nil && closeErr != nil {
			err = fmt.Errorf("close target %s: %w", targetPath, closeErr)
		}
	}()
	sectors, _, err := v.scan()
	if err != nil {
		return err
	}
	for si := range sectors {
		for ei, e := range sectors[si].entries {
			if e.startingSector != lastSector && e.present && keysEqual(e, key) {
				sectors[si].entries[ei].present = false
				if err := v.writeSector(sectors[si].address, encodeFATSector(sectors[si])); err != nil {
					return err
				}
				if err := v.sync(); err != nil {
					return err
				}
				_, err = fmt.Fprintf(out, "removed %s from %s\n", keyDisplay(key), targetPath)
				return err
			}
		}
	}
	return fmt.Errorf("file %s was not found", keyDisplay(key))
}

func runExtract(targetPath, outputPath string, key fileKey, out io.Writer) (err error) {
	v, err := openReadOnly(targetPath)
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := v.close(); err == nil && closeErr != nil {
			err = fmt.Errorf("close target %s: %w", targetPath, closeErr)
		}
	}()

	sectors, _, err := v.scan()
	if err != nil {
		return err
	}
	var found *entry
	for si := range sectors {
		for ei := range sectors[si].entries {
			e := &sectors[si].entries[ei]
			if e.startingSector != lastSector && e.present && keysEqual(*e, key) {
				found = e
				break
			}
		}
		if found != nil {
			break
		}
	}
	if found == nil {
		return fmt.Errorf("file %s was not found", keyDisplay(key))
	}

	output, err := os.OpenFile(outputPath, os.O_WRONLY|os.O_CREATE|os.O_EXCL, 0666)
	if err != nil {
		return fmt.Errorf("create output %s: %w", outputPath, err)
	}
	complete := false
	defer func() {
		if !complete {
			_ = output.Close()
			_ = os.Remove(outputPath)
		}
	}()

	size := entrySizeBytes(*found)
	if err := copyBytesAt(output, 0, v.file, int64(found.startingSector)*int64(sectorSize), size); err != nil {
		return fmt.Errorf("extract %s: %w", keyDisplay(key), err)
	}
	if err := output.Close(); err != nil {
		return fmt.Errorf("close output %s: %w", outputPath, err)
	}
	complete = true
	_, err = fmt.Fprintf(out, "extracted %s from %s to %s: %d bytes\n", keyDisplay(key), targetPath, outputPath, size)
	return err
}

func runList(targetPath string, includeRemoved bool, out io.Writer) (err error) {
	v, err := openReadOnly(targetPath)
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := v.close(); err == nil && closeErr != nil {
			err = fmt.Errorf("close target %s: %w", targetPath, closeErr)
		}
	}()
	sectors, _, err := v.scan()
	if err != nil {
		return err
	}

	table := tabwriter.NewWriter(out, 0, 4, 2, ' ', 0)
	if includeRemoved {
		_, err = fmt.Fprintln(table, "MODE\tTYPE\tNAME\tBYTES\tSTART\tSECTORS\tLAST_BYTES\tMETA\tSTATE")
	} else {
		_, err = fmt.Fprintln(table, "MODE\tTYPE\tNAME\tBYTES\tSTART\tSECTORS\tLAST_BYTES\tMETA")
	}
	if err != nil {
		return err
	}
	for _, sector := range sectors {
		for _, e := range sector.entries {
			if e.startingSector == lastSector || (!includeRemoved && !e.present) {
				continue
			}
			name := strings.TrimRight(string(e.name[:]), "\x00")
			if includeRemoved {
				state := "removed"
				if e.present {
					state = "live"
				}
				_, err = fmt.Fprintf(table, "0x%02x\t%s\t%s\t%d\t%d\t%d\t%d\t%s\t%s\n",
					e.modeID, typeDisplay(e.typeID), name, entrySizeBytes(e), e.startingSector,
					e.sectorCount, e.lastBytes, hex.EncodeToString(e.metadata[:]), state)
			} else {
				_, err = fmt.Fprintf(table, "0x%02x\t%s\t%s\t%d\t%d\t%d\t%d\t%s\n",
					e.modeID, typeDisplay(e.typeID), name, entrySizeBytes(e), e.startingSector,
					e.sectorCount, e.lastBytes, hex.EncodeToString(e.metadata[:]))
			}
			if err != nil {
				return err
			}
		}
	}
	return table.Flush()
}

func typeDisplay(typeID uint8) string {
	switch typeID {
	case typePCM:
		return "pcm"
	case typeFirmware:
		return "firmware"
	default:
		return fmt.Sprintf("0x%02x", typeID)
	}
}

type stagedEntry struct {
	entry  entry
	offset int64
}

func runDefrag(targetPath string, out io.Writer) (err error) {
	v, err := openExisting(targetPath)
	if err != nil {
		return err
	}
	defer func() {
		if closeErr := v.close(); err == nil && closeErr != nil {
			err = fmt.Errorf("close target %s: %w", targetPath, closeErr)
		}
	}()
	sectors, oldHighWater, err := v.scan()
	if err != nil {
		return err
	}
	temp, err := os.CreateTemp("", "ktane-fat-defrag-*")
	if err != nil {
		return fmt.Errorf("create defrag staging file: %w", err)
	}
	tempPath := temp.Name()
	defer func() {
		_ = temp.Close()
		_ = os.Remove(tempPath)
	}()

	var live []stagedEntry
	var stageOffset int64
	for _, sector := range sectors {
		for _, e := range sector.entries {
			if e.startingSector == lastSector || !e.present {
				continue
			}
			staged := stagedEntry{entry: e, offset: stageOffset}
			staged.entry.reserved = 0
			allocationBytes := uint64(e.sectorCount) * sectorSize
			if allocationBytes != 0 {
				if err := copyBytesAt(temp, stageOffset, v.file, int64(e.startingSector)*int64(sectorSize), allocationBytes); err != nil {
					return fmt.Errorf("stage %s: %w", keyDisplay(fileKey{e.modeID, e.typeID, e.name}), err)
				}
				stageOffset += int64(allocationBytes)
			}
			live = append(live, staged)
		}
	}

	var rebuilt []fatSector
	address := uint64(0)
	for first := 0; first < len(live) || first == 0; first += maxEntries {
		fatAddress := address
		address++
		s := emptyFATSector()
		s.address = uint32(fatAddress)
		limit := first + maxEntries
		if limit > len(live) {
			limit = len(live)
		}
		for i := first; i < limit; i++ {
			e := live[i].entry
			if e.sectorCount == 0 {
				e.startingSector = 0
			} else {
				if address >= uint64(lastSector) || address+uint64(e.sectorCount) > uint64(v.sectors) {
					return errors.New("compacted data does not fit target")
				}
				e.startingSector = uint32(address)
				address += uint64(e.sectorCount)
			}
			live[i].entry = e
			s.entries[i-first] = e
		}
		rebuilt = append(rebuilt, s)
		if limit == len(live) {
			break
		}
	}
	if address > uint64(v.sectors) {
		return errors.New("compacted data does not fit target")
	}
	for i := range rebuilt {
		if i+1 < len(rebuilt) {
			rebuilt[i].next = rebuilt[i+1].address
		}
	}

	for _, staged := range live {
		allocationBytes := uint64(staged.entry.sectorCount) * sectorSize
		if allocationBytes == 0 {
			continue
		}
		if err := copyBytesAt(v.file, int64(staged.entry.startingSector)*int64(sectorSize), temp, staged.offset, allocationBytes); err != nil {
			return fmt.Errorf("restore %s: %w", keyDisplay(fileKey{staged.entry.modeID, staged.entry.typeID, staged.entry.name}), err)
		}
	}
	for i := 1; i < len(rebuilt); i++ {
		if err := v.writeSector(rebuilt[i].address, encodeFATSector(rebuilt[i])); err != nil {
			return err
		}
	}
	if err := v.writeSector(0, encodeFATSector(rebuilt[0])); err != nil {
		return err
	}
	if err := v.sync(); err != nil {
		return err
	}
	newHighWater := uint32(address - 1)
	reclaimed := uint32(0)
	if oldHighWater > newHighWater {
		reclaimed = oldHighWater - newHighWater
	}
	_, err = fmt.Fprintf(out, "defragmented %s: %d files, %d sectors reclaimed\n", targetPath, len(live), reclaimed)
	return err
}
