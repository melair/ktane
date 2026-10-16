package fat

import (
	"bytes"
	"encoding/binary"
	"errors"
	"io"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestNamedTypesAndModeFormats(t *testing.T) {
	for value, want := range map[string]uint8{"pcm": typePCM, "firmware": typeFirmware} {
		got, err := parseType(value)
		if err != nil || got != want {
			t.Fatalf("parseType(%q) = %#x, %v; want %#x", value, got, err, want)
		}
	}
	for _, value := range []string{"1", "0x01", "PCM", "fw", "unknown"} {
		if _, err := parseType(value); err == nil {
			t.Fatalf("parseType(%q) succeeded", value)
		}
	}
	for value, want := range map[string]uint8{"66": 0x42, "0x42": 0x42, "0xc1": 0xc1} {
		got, err := parseByte("--mode", value)
		if err != nil || got != want {
			t.Fatalf("parseByte(%q) = %#x, %v; want %#x", value, got, err, want)
		}
	}
}

func TestAddKeyInference(t *testing.T) {
	tests := []struct {
		name     string
		opts     keyOptions
		source   string
		wantType uint8
		wantName string
		wantErr  string
	}{
		{name: "pcm", opts: keyOptions{mode: "0x42"}, source: "/sounds/beeps.pcm", wantType: typePCM, wantName: "beeps"},
		{name: "firmware", opts: keyOptions{mode: "193"}, source: "release.fw", wantType: typeFirmware, wantName: "release"},
		{name: "explicit type", opts: keyOptions{mode: "1", typeID: "firmware"}, source: "release.v2.bin", wantType: typeFirmware, wantName: "release.v2"},
		{name: "explicit name", opts: keyOptions{mode: "1", name: "main"}, source: "release.fw", wantType: typeFirmware, wantName: "main"},
		{name: "explicit type with uppercase extension", opts: keyOptions{mode: "1", typeID: "pcm"}, source: "beeps.PCM", wantType: typePCM, wantName: "beeps"},
		{name: "unsupported extension", opts: keyOptions{mode: "1"}, source: "beeps.wav", wantErr: "--type is required"},
		{name: "uppercase inferred extension", opts: keyOptions{mode: "1"}, source: "beeps.PCM", wantErr: "--type is required"},
		{name: "long inferred name", opts: keyOptions{mode: "1"}, source: "this-is-too-long.pcm", wantErr: "maximum is 10"},
	}
	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			got, err := parseAddKey(test.opts, test.source)
			if test.wantErr != "" {
				if err == nil || !strings.Contains(err.Error(), test.wantErr) {
					t.Fatalf("error = %v, want containing %q", err, test.wantErr)
				}
				return
			}
			if err != nil {
				t.Fatal(err)
			}
			if got.typeID != test.wantType || keyDisplay(got) != test.wantName {
				t.Fatalf("key = type %#x name %q", got.typeID, keyDisplay(got))
			}
		})
	}
}

func TestFATSectorEncodingMatchesFirmwareLayout(t *testing.T) {
	s := emptyFATSector()
	s.address = 0
	s.next = 99
	s.entries[0] = entry{
		startingSector: 7,
		sectorCount:    2,
		lastBytes:      512,
		present:        true,
		reserved:       0x15,
		modeID:         3,
		typeID:         4,
		metadata:       [4]byte{0x10, 0x20, 0x30, 0x40},
		name:           [10]byte{'T', 'E', 'S', 'T'},
	}
	data := encodeFATSector(s)
	if got := binary.LittleEndian.Uint32(data[0:4]); got != magic {
		t.Fatalf("magic = %#x", got)
	}
	if got := binary.LittleEndian.Uint32(data[4:8]); got != 99 {
		t.Fatalf("next = %d", got)
	}
	if got := binary.LittleEndian.Uint16(data[14:16]); got != 0xae00 {
		t.Fatalf("flags = %#x, want %#x", got, 0xae00)
	}
	if !bytes.Equal(data[16:32], []byte{3, 4, 0x10, 0x20, 0x30, 0x40, 'T', 'E', 'S', 'T', 0, 0, 0, 0, 0, 0}) {
		t.Fatalf("key and metadata bytes = %x", data[16:32])
	}
	decoded, err := decodeFATSector(0, data)
	if err != nil {
		t.Fatal(err)
	}
	if decoded.entries[0] != s.entries[0] {
		t.Fatalf("decoded entry = %#v, want %#v", decoded.entries[0], s.entries[0])
	}
	if decoded.entries[1].startingSector != lastSector {
		t.Fatal("unused entry was not encoded with FAT_LAST_SECTOR")
	}
}

func TestFormatCreatesImageAndPreservesExistingCapacity(t *testing.T) {
	dir := t.TempDir()
	created := filepath.Join(dir, "created.img")
	if err := runFormat(created, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	info, err := os.Stat(created)
	if err != nil {
		t.Fatal(err)
	}
	if info.Size() != 1024*1024 {
		t.Fatalf("created size = %d", info.Size())
	}
	assertEmptyRoot(t, created)
	if err := runFormat(created, 1, true, io.Discard); err == nil {
		t.Fatal("--size accepted for existing image")
	}

	existing := filepath.Join(dir, "existing.img")
	original := bytes.Repeat([]byte{0xaa}, 2*int(sectorSize))
	if err := os.WriteFile(existing, original, 0644); err != nil {
		t.Fatal(err)
	}
	if err := runFormat(existing, 0, false, io.Discard); err != nil {
		t.Fatal(err)
	}
	got, err := os.ReadFile(existing)
	if err != nil {
		t.Fatal(err)
	}
	if len(got) != len(original) || !bytes.Equal(got[sectorSize:], original[sectorSize:]) {
		t.Fatal("format resized the image or changed sectors after the root")
	}
	if err := runFormat(filepath.Join(dir, "missing.img"), 0, false, io.Discard); err == nil {
		t.Fatal("missing image accepted without --size")
	}
}

func TestAddReplaceRemoveAndDefrag(t *testing.T) {
	path := filepath.Join(t.TempDir(), "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	keyA := testKey(1, 2, "A")
	keyB := testKey(1, 2, "B")
	dataA := bytes.Repeat([]byte{0x41}, 600)
	dataB := bytes.Repeat([]byte{0x42}, 700)
	addTestFile(t, path, keyA, [4]byte{1, 2, 3, 4}, dataA)
	addTestFile(t, path, keyB, [4]byte{5, 6, 7, 8}, dataB)

	v := openTestVolume(t, path)
	sectors, highWater, err := v.scan()
	if err != nil {
		t.Fatal(err)
	}
	if highWater != 4 || sectors[0].entries[0].startingSector != 1 || sectors[0].entries[1].startingSector != 3 {
		t.Fatalf("unexpected initial allocation: high=%d entries=%#v", highWater, sectors[0].entries[:2])
	}
	_ = v.close()

	if err := runRemove(path, keyA, io.Discard); err != nil {
		t.Fatal(err)
	}
	v = openTestVolume(t, path)
	sectors, highWater, err = v.scan()
	if err != nil {
		t.Fatal(err)
	}
	if sectors[0].entries[0].present || sectors[0].entries[0].startingSector != 1 || highWater != 4 {
		t.Fatal("remove did not leave a tombstoned allocation")
	}
	_ = v.close()

	if err := runDefrag(path, io.Discard); err != nil {
		t.Fatal(err)
	}
	v = openTestVolume(t, path)
	sectors, highWater, err = v.scan()
	if err != nil {
		t.Fatal(err)
	}
	if highWater != 2 || len(sectors) != 1 {
		t.Fatalf("defrag high water = %d, FAT sectors = %d", highWater, len(sectors))
	}
	e := sectors[0].entries[0]
	if !e.present || !keysEqual(e, keyB) || e.startingSector != 1 || e.metadata != [4]byte{5, 6, 7, 8} {
		t.Fatalf("defragged entry = %#v", e)
	}
	if sectors[0].entries[1].startingSector != lastSector {
		t.Fatal("defrag retained the tombstone")
	}
	payload := make([]byte, len(dataB))
	if _, err := v.file.ReadAt(payload, int64(e.startingSector)*int64(sectorSize)); err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(payload, dataB) {
		t.Fatal("defrag changed the live payload")
	}

	replacement := bytes.Repeat([]byte{0x43}, 300)
	_ = v.close()
	addTestFile(t, path, keyB, [4]byte{9, 8, 7, 6}, replacement)
	v = openTestVolume(t, path)
	sectors, _, err = v.scan()
	if err != nil {
		t.Fatal(err)
	}
	e = sectors[0].entries[0]
	if e.startingSector != 1 || entrySizeBytes(e) != uint64(len(replacement)) || e.metadata != [4]byte{9, 8, 7, 6} {
		t.Fatalf("replacement entry = %#v", e)
	}
	_ = v.close()
}

func TestAddExtendsFATChainAtTwentySecondEntry(t *testing.T) {
	path := filepath.Join(t.TempDir(), "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 22; i++ {
		key := testKey(1, 0, string([]byte{'A' + byte(i)}))
		addTestFile(t, path, key, [4]byte{}, nil)
	}
	v := openTestVolume(t, path)
	sectors, highWater, err := v.scan()
	if err != nil {
		t.Fatal(err)
	}
	if len(sectors) != 2 || sectors[0].next != 1 || sectors[1].address != 1 || highWater != 1 {
		t.Fatalf("FAT chain = %#v, high water = %d", sectors, highWater)
	}
	if !sectors[1].entries[0].present || sectors[1].entries[0].startingSector != 0 {
		t.Fatalf("22nd entry = %#v", sectors[1].entries[0])
	}
	if err := v.close(); err != nil {
		t.Fatal(err)
	}
	var listing bytes.Buffer
	if err := runList(path, false, &listing); err != nil {
		t.Fatal(err)
	}
	if lines := strings.Split(strings.TrimSpace(listing.String()), "\n"); len(lines) != 23 {
		t.Fatalf("listed %d rows across two FAT sectors, want 22 entries plus header:\n%s", len(lines), listing.String())
	}
}

func TestExtractWritesExactPayloadAndProtectsExistingOutput(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	key := testKey(0x42, typePCM, "beeps")
	payload := bytes.Repeat([]byte{0x5a}, int(sectorSize)+37)
	addTestFile(t, path, key, [4]byte{}, payload)

	output := filepath.Join(dir, "beeps.pcm")
	if err := runExtract(path, output, key, io.Discard); err != nil {
		t.Fatal(err)
	}
	got, err := os.ReadFile(output)
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Equal(got, payload) {
		t.Fatalf("extracted %d bytes, want exact %d-byte payload", len(got), len(payload))
	}

	if err := runExtract(path, output, key, io.Discard); err == nil {
		t.Fatal("extract overwrote an existing output")
	}
	got, err = os.ReadFile(output)
	if err != nil || !bytes.Equal(got, payload) {
		t.Fatalf("existing output changed: %v", err)
	}
}

func TestExtractHandlesEmptyMissingAndRemovedFiles(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	emptyKey := testKey(1, typeFirmware, "empty")
	addTestFile(t, path, emptyKey, [4]byte{}, nil)
	emptyOutput := filepath.Join(dir, "empty.fw")
	if err := runExtract(path, emptyOutput, emptyKey, io.Discard); err != nil {
		t.Fatal(err)
	}
	if info, err := os.Stat(emptyOutput); err != nil || info.Size() != 0 {
		t.Fatalf("empty output: info=%v err=%v", info, err)
	}

	missingOutput := filepath.Join(dir, "missing.pcm")
	if err := runExtract(path, missingOutput, testKey(1, typePCM, "missing"), io.Discard); err == nil {
		t.Fatal("missing file extracted")
	}
	if _, err := os.Stat(missingOutput); !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("missing extraction created output: %v", err)
	}

	if err := runRemove(path, emptyKey, io.Discard); err != nil {
		t.Fatal(err)
	}
	removedOutput := filepath.Join(dir, "removed.fw")
	if err := runExtract(path, removedOutput, emptyKey, io.Discard); err == nil {
		t.Fatal("removed file extracted")
	}
	if _, err := os.Stat(removedOutput); !errors.Is(err, os.ErrNotExist) {
		t.Fatalf("removed extraction created output: %v", err)
	}
}

func TestListShowsLiveEntriesAndOptionallyRemovedEntries(t *testing.T) {
	path := filepath.Join(t.TempDir(), "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	liveKey := testKey(0x42, typePCM, "beeps")
	alignedKey := testKey(0x42, typePCM, "aligned")
	removedKey := testKey(0xc1, 0x7e, "old")
	addTestFile(t, path, liveKey, [4]byte{1, 2, 0xab, 0xcd}, bytes.Repeat([]byte{1}, 600))
	addTestFile(t, path, alignedKey, [4]byte{}, bytes.Repeat([]byte{2}, int(sectorSize)))
	addTestFile(t, path, removedKey, [4]byte{4, 3, 2, 1}, nil)
	if err := runRemove(path, removedKey, io.Discard); err != nil {
		t.Fatal(err)
	}

	var live bytes.Buffer
	if err := runList(path, false, &live); err != nil {
		t.Fatal(err)
	}
	listing := live.String()
	for _, want := range []string{"MODE", "TYPE", "NAME", "BYTES", "START", "SECTORS", "LAST_BYTES", "META", "0x42", "pcm", "beeps", "600", "0102abcd"} {
		if !strings.Contains(listing, want) {
			t.Fatalf("listing missing %q:\n%s", want, listing)
		}
	}
	if row := listingRow(t, listing, "beeps"); row[6] != "88" {
		t.Fatalf("partial-sector LAST_BYTES = %s, want 88", row[6])
	}
	if row := listingRow(t, listing, "aligned"); row[6] != "512" {
		t.Fatalf("aligned LAST_BYTES = %s, want 512", row[6])
	}
	if strings.Contains(listing, "old") || strings.Contains(listing, "STATE") {
		t.Fatalf("default listing exposed removed entries:\n%s", listing)
	}

	var all bytes.Buffer
	if err := runList(path, true, &all); err != nil {
		t.Fatal(err)
	}
	listing = all.String()
	for _, want := range []string{"STATE", "live", "removed", "0xc1", "0x7e", "old", "04030201"} {
		if !strings.Contains(listing, want) {
			t.Fatalf("--all listing missing %q:\n%s", want, listing)
		}
	}
	if row := listingRow(t, listing, "old"); row[6] != "0" {
		t.Fatalf("zero-byte LAST_BYTES = %s, want 0", row[6])
	}
	if strings.Index(listing, "beeps") > strings.Index(listing, "old") {
		t.Fatalf("listing did not preserve FAT order:\n%s", listing)
	}
}

func listingRow(t *testing.T, listing, name string) []string {
	t.Helper()
	for _, line := range strings.Split(listing, "\n") {
		fields := strings.Fields(line)
		if len(fields) >= 3 && fields[2] == name {
			return fields
		}
	}
	t.Fatalf("listing has no row named %q:\n%s", name, listing)
	return nil
}

func TestListEmptyFATPrintsHeaders(t *testing.T) {
	path := filepath.Join(t.TempDir(), "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	var output bytes.Buffer
	if err := runList(path, false, &output); err != nil {
		t.Fatal(err)
	}
	if got := strings.TrimSpace(output.String()); !strings.HasPrefix(got, "MODE") || strings.Contains(got, "\n") {
		t.Fatalf("empty listing = %q", got)
	}
}

func TestExtractAndListAcceptReadOnlyImage(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "card.img")
	if err := runFormat(path, 1, true, io.Discard); err != nil {
		t.Fatal(err)
	}
	key := testKey(0x42, typePCM, "beeps")
	payload := []byte("read-only image payload")
	addTestFile(t, path, key, [4]byte{}, payload)
	if err := os.Chmod(path, 0444); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = os.Chmod(path, 0644) })

	if err := runList(path, false, io.Discard); err != nil {
		t.Fatalf("list read-only image: %v", err)
	}
	output := filepath.Join(dir, "beeps.pcm")
	if err := runExtract(path, output, key, io.Discard); err != nil {
		t.Fatalf("extract read-only image: %v", err)
	}
	got, err := os.ReadFile(output)
	if err != nil || !bytes.Equal(got, payload) {
		t.Fatalf("extracted payload = %q, %v", got, err)
	}
}

func TestScanRejectsCorruption(t *testing.T) {
	tests := map[string]func(*volume) error{
		"bad magic": func(v *volume) error {
			return v.writeSector(0, make([]byte, sectorSize))
		},
		"self link": func(v *volume) error {
			root := emptyFATSector()
			root.next = 0
			return v.writeSector(0, encodeFATSector(root))
		},
		"out of bounds link": func(v *volume) error {
			root := emptyFATSector()
			root.next = v.sectors
			return v.writeSector(0, encodeFATSector(root))
		},
		"malformed size": func(v *volume) error {
			root := emptyFATSector()
			root.entries[0] = entry{startingSector: 1, sectorCount: 1, lastBytes: 0, present: true}
			return v.writeSector(0, encodeFATSector(root))
		},
		"out of bounds allocation": func(v *volume) error {
			root := emptyFATSector()
			root.entries[0] = entry{startingSector: v.sectors, sectorCount: 1, lastBytes: 1, present: true}
			return v.writeSector(0, encodeFATSector(root))
		},
	}
	for name, corrupt := range tests {
		t.Run(name, func(t *testing.T) {
			path := filepath.Join(t.TempDir(), "bad.img")
			if err := runFormat(path, 1, true, io.Discard); err != nil {
				t.Fatal(err)
			}
			v := openTestVolume(t, path)
			defer v.close()
			if err := corrupt(v); err != nil {
				t.Fatal(err)
			}
			if _, _, err := v.scan(); err == nil {
				t.Fatal("corrupt FAT accepted")
			}
		})
	}
}

func testKey(mode, typeID uint8, name string) fileKey {
	key := fileKey{modeID: mode, typeID: typeID}
	copy(key.name[:], name)
	return key
}

func addTestFile(t *testing.T, target string, key fileKey, metadata [4]byte, data []byte) {
	t.Helper()
	source := filepath.Join(t.TempDir(), "source.bin")
	if err := os.WriteFile(source, data, 0644); err != nil {
		t.Fatal(err)
	}
	if err := runAdd(target, source, key, metadata, io.Discard); err != nil {
		t.Fatal(err)
	}
}

func openTestVolume(t *testing.T, path string) *volume {
	t.Helper()
	v, err := openExisting(path)
	if err != nil {
		t.Fatal(err)
	}
	return v
}

func assertEmptyRoot(t *testing.T, path string) {
	t.Helper()
	v := openTestVolume(t, path)
	defer v.close()
	sectors, highWater, err := v.scan()
	if err != nil {
		t.Fatal(err)
	}
	if len(sectors) != 1 || highWater != 0 || sectors[0].next != lastSector {
		t.Fatalf("unexpected empty FAT: %#v high=%d", sectors, highWater)
	}
	for _, e := range sectors[0].entries {
		if e.startingSector != lastSector {
			t.Fatal("empty FAT contains an allocated entry")
		}
	}
}
