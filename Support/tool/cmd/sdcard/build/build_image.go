package build

import (
	"fmt"
	"github.com/melair/ktane/common"
	"github.com/shimmeringbee/bytecodec"
	"io"
	"io/fs"
	"log"
	"math"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
)

var fileRegex = regexp.MustCompile("(\\d+)\\S*[\\\\\\/](\\d+)\\S*\\.([a-z]+)$")

type foundFile struct {
	path      string
	mode      int
	id        int
	ext       string
	sizeBytes int64
}

func buildImage(image string, resources string) error {
	log.Printf("building image, resources: %s image: %s", resources, image)

	files, err := enumerateFiles(resources)
	if err != nil {
		return fmt.Errorf("enumerate files: %w", err)
	}

	files = filterFiles(files)

	w, err := os.Create(image)
	if err != nil {
		return fmt.Errorf("create output file: %w", err)
	}

	defer func() {
		if err := w.Close(); err != nil {
			log.Printf("failed to close file: %s", err.Error())
		}
	}()

	fatSectors, err := writeIndex(w, files)
	if err != nil {
		return fmt.Errorf("writing index: %w", err)
	}

	log.Printf("written %d fat sectors", fatSectors)

	if err := writeFiles(w, files, fatSectors); err != nil {
		return fmt.Errorf("writing files: %w", err)
	}

	return nil
}

func writeFiles(w io.Writer, files []foundFile, startingSector int) error {
	for _, ff := range files {
		if blocks, err := writeFile(w, ff, startingSector); err != nil {
			return fmt.Errorf("write file: %s: %w", ff.path, err)
		} else {
			startingSector += blocks
		}
	}

	return nil
}

func writeFile(w io.Writer, file foundFile, startingSector int) (int, error) {
	r, err := os.Open(file.path)
	if err != nil {
		return 0, fmt.Errorf("failed to open file: %w", err)
	}

	defer func() {
		if err := r.Close(); err != nil {
			log.Printf("failed to close beign read file: %s: %s", file.path, err.Error())
		}
	}()

	sectorSize := 512

	if file.ext == "pcm" {
		sectorSize = 500
	}

	sectors := int(math.Ceil(float64(file.sizeBytes) / float64(sectorSize)))
	paddingSize := 512 - sectorSize

	data := make([]byte, sectorSize)
	padding := make([]byte, paddingSize)

	for i := 0; i < sectors; i++ {
		n, err := r.Read(data)
		if err != nil {
			return 0, fmt.Errorf("read failed: %w", err)
		}

		if n != sectorSize {
			log.Printf("did not read whole sector, padding with 0x7f: %s: %d != %d", file.path, n, sectorSize)

			for j := n; j < sectorSize; j++ {
				data[j] = 0x7f
			}
		}

		if n, err := w.Write(data); err != nil {
			return 0, fmt.Errorf("write failed: %w", err)
		} else if n != sectorSize {
			return 0, fmt.Errorf("write failed: did not write hole block: %d != %d", n, sectorSize)
		}

		if n, err := w.Write(padding); err != nil {
			return 0, fmt.Errorf("write failed (padding): %w", err)
		} else if n != paddingSize {
			return 0, fmt.Errorf("write failed (padding): did not write hole block: %d != %d", n, paddingSize)
		}
	}

	log.Printf("written file: %s: sector: %d sectorLength: %d", file.path, startingSector, sectors)

	return sectors, nil
}

const FatEntriesPerSector = 64

func writeIndex(w io.Writer, files []foundFile) (int, error) {
	fatEntryCount := len(files) + 1
	fatSectors := int(math.Ceil(float64(fatEntryCount) / FatEntriesPerSector))
	fatEntries := fatSectors * FatEntriesPerSector

	fat := make([]common.FATEntry, fatEntries)

	for i, file := range files {
		fat[i] = fileToFAT(file)
	}

	fatData, err := bytecodec.Marshal(fat)
	if err != nil {
		return 0, fmt.Errorf("failed to marshal fat: %w", err)
	}

	written, err := w.Write(fatData)
	if err != nil {
		return 0, fmt.Errorf("failed to write fat: %w", err)
	} else if written != len(fatData) {
		return 0, fmt.Errorf("failed to write fat: missing bytes %d != %d", written, len(fatData))
	}

	return fatSectors, nil
}

var extToType = map[string]uint8{
	"eoi": 0,
	"pcm": 1,
}

func fileToFAT(file foundFile) common.FATEntry {
	fatEntry := common.FATEntry{
		Module: uint8(file.mode),
		Index:  uint8(file.id),
		Type:   extToType[file.ext],
	}

	switch file.ext {
	case "pcm":
		fatEntry.Size = uint16(math.Ceil(float64(file.sizeBytes) / 500))
	default:
		fatEntry.Size = uint16(math.Ceil(float64(file.sizeBytes) / 512))
	}

	return fatEntry
}

func enumerateFiles(resources string) ([]foundFile, error) {
	var files []foundFile

	if err := filepath.Walk(resources, func(path string, info fs.FileInfo, err error) error {
		if err != nil {
			return fmt.Errorf("walk error: %w", err)
		}

		if info.IsDir() {
			return nil
		}

		matched := fileRegex.FindAllStringSubmatch(path, -1)

		if len(matched) != 1 || len(matched[0]) != 4 {
			log.Printf("ignoring file, missing path components: %s", path)
			return nil
		}

		mode, err := strconv.Atoi(matched[0][1])
		if err != nil {
			log.Printf("ignoring file, invalid mode: %s", path)
			return nil
		}

		id, err := strconv.Atoi(matched[0][2])
		if err != nil {
			log.Printf("ignoring file, invalid id: %s", path)
			return nil
		}

		ext := matched[0][3]

		files = append(files, foundFile{
			path:      path,
			mode:      mode,
			id:        id,
			ext:       ext,
			sizeBytes: info.Size(),
		})

		return nil
	}); err != nil {
		return nil, fmt.Errorf("failed to walk resources directory: %w", err)
	}

	return files, nil
}

func filterFiles(input []foundFile) []foundFile {
	var output []foundFile

	for _, file := range input {
		switch file.ext {
		case "pcm":
			output = append(output, file)
		default:
			log.Printf("unknown file extension: %s", file.path)
		}
	}

	return output
}
