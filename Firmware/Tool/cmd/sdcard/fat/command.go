package fat

import (
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/spf13/cobra"
)

type keyOptions struct {
	mode   string
	typeID string
	name   string
}

type fileKey struct {
	modeID uint8
	typeID uint8
	name   [nameSize]byte
}

const (
	typePCM      = uint8(0x01)
	typeFirmware = uint8(0xff)
)

// NewCommand creates the custom FAT command group.
func NewCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "fat",
		Short: "Work with the KTANE SD card file allocation table",
	}
	cmd.AddCommand(newFormatCommand(), newAddCommand(), newRemoveCommand(), newExtractCommand(), newListCommand(), newDefragCommand())
	return cmd
}

func newFormatCommand() *cobra.Command {
	var sizeMiB uint64
	cmd := &cobra.Command{
		Use:   "format <target>",
		Short: "Write a new empty file allocation table",
		Args:  cobra.ExactArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return runFormat(args[0], sizeMiB, cmd.Flags().Changed("size"), cmd.OutOrStdout())
		},
	}
	cmd.Flags().Uint64Var(&sizeMiB, "size", 0, "New image size in MiB (required when target does not exist)")
	return cmd
}

func newAddCommand() *cobra.Command {
	var key keyOptions
	var metadata string
	cmd := &cobra.Command{
		Use:   "add <target> <source>",
		Short: "Add or replace one file",
		Args:  cobra.ExactArgs(2),
		RunE: func(cmd *cobra.Command, args []string) error {
			parsedKey, err := parseAddKey(key, args[1])
			if err != nil {
				return err
			}
			parsedMetadata, err := parseMetadata(metadata)
			if err != nil {
				return err
			}
			return runAdd(args[0], args[1], parsedKey, parsedMetadata, cmd.OutOrStdout())
		},
	}
	addKeyFlags(cmd, &key, false)
	cmd.Flags().StringVar(&metadata, "meta", "00000000", "Four metadata bytes as exactly eight hexadecimal digits")
	return cmd
}

func newRemoveCommand() *cobra.Command {
	var key keyOptions
	cmd := &cobra.Command{
		Use:   "remove <target>",
		Short: "Tombstone one file",
		Args:  cobra.ExactArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			parsedKey, err := parseKey(key)
			if err != nil {
				return err
			}
			return runRemove(args[0], parsedKey, cmd.OutOrStdout())
		},
	}
	addKeyFlags(cmd, &key, true)
	return cmd
}

func newExtractCommand() *cobra.Command {
	var key keyOptions
	cmd := &cobra.Command{
		Use:   "extract <target> <output>",
		Short: "Extract one file",
		Args:  cobra.ExactArgs(2),
		RunE: func(cmd *cobra.Command, args []string) error {
			parsedKey, err := parseKey(key)
			if err != nil {
				return err
			}
			return runExtract(args[0], args[1], parsedKey, cmd.OutOrStdout())
		},
	}
	addKeyFlags(cmd, &key, true)
	return cmd
}

func newListCommand() *cobra.Command {
	var includeRemoved bool
	cmd := &cobra.Command{
		Use:   "list <target>",
		Short: "List file allocation table entries",
		Args:  cobra.ExactArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return runList(args[0], includeRemoved, cmd.OutOrStdout())
		},
	}
	cmd.Flags().BoolVar(&includeRemoved, "all", false, "Include removed entries")
	return cmd
}

func newDefragCommand() *cobra.Command {
	return &cobra.Command{
		Use:   "defrag <target>",
		Short: "Compact files and reclaim unused allocations",
		Args:  cobra.ExactArgs(1),
		RunE: func(cmd *cobra.Command, args []string) error {
			return runDefrag(args[0], cmd.OutOrStdout())
		},
	}
}

func addKeyFlags(cmd *cobra.Command, opts *keyOptions, requireCompleteKey bool) {
	typeHelp := "File type: pcm or firmware"
	nameHelp := "File name of at most 10 bytes"
	if !requireCompleteKey {
		typeHelp += " (inferred from .pcm or .fw when omitted)"
		nameHelp += " (inferred from the source basename when omitted)"
	}
	cmd.Flags().StringVar(&opts.mode, "mode", "", "Mode ID (decimal or 0x-prefixed)")
	cmd.Flags().StringVar(&opts.typeID, "type", "", typeHelp)
	cmd.Flags().StringVar(&opts.name, "name", "", nameHelp)
	_ = cmd.MarkFlagRequired("mode")
	if requireCompleteKey {
		_ = cmd.MarkFlagRequired("type")
		_ = cmd.MarkFlagRequired("name")
	}
}

func parseKey(opts keyOptions) (fileKey, error) {
	mode, err := parseByte("--mode", opts.mode)
	if err != nil {
		return fileKey{}, err
	}
	typeID, err := parseType(opts.typeID)
	if err != nil {
		return fileKey{}, err
	}
	return makeKey(mode, typeID, opts.name)
}

func parseAddKey(opts keyOptions, source string) (fileKey, error) {
	mode, err := parseByte("--mode", opts.mode)
	if err != nil {
		return fileKey{}, err
	}
	typeName := opts.typeID
	if typeName == "" {
		switch filepath.Ext(source) {
		case ".pcm":
			typeName = "pcm"
		case ".fw":
			typeName = "firmware"
		default:
			return fileKey{}, errors.New("--type is required when the source extension is not .pcm or .fw")
		}
	}
	typeID, err := parseType(typeName)
	if err != nil {
		return fileKey{}, err
	}
	name := opts.name
	if name == "" {
		base := filepath.Base(source)
		name = strings.TrimSuffix(base, filepath.Ext(base))
	}
	return makeKey(mode, typeID, name)
}

func makeKey(mode, typeID uint8, name string) (fileKey, error) {
	if name == "" {
		return fileKey{}, errors.New("--name must not be empty")
	}
	if strings.IndexByte(name, 0) >= 0 {
		return fileKey{}, errors.New("--name must not contain NUL bytes")
	}
	if len(name) > nameSize {
		return fileKey{}, fmt.Errorf("--name is %d bytes; maximum is %d", len(name), nameSize)
	}
	key := fileKey{modeID: mode, typeID: typeID}
	copy(key.name[:], name)
	return key, nil
}

func parseType(value string) (uint8, error) {
	switch value {
	case "pcm":
		return typePCM, nil
	case "firmware":
		return typeFirmware, nil
	default:
		return 0, fmt.Errorf("--type must be one of: pcm, firmware")
	}
}

func parseByte(flag, value string) (uint8, error) {
	parsed, err := strconv.ParseUint(value, 0, 8)
	if err != nil {
		return 0, fmt.Errorf("%s must be an integer from 0 to 255: %w", flag, err)
	}
	return uint8(parsed), nil
}

func parseMetadata(value string) ([metadataSize]byte, error) {
	var metadata [metadataSize]byte
	if len(value) != metadataSize*2 {
		return metadata, errors.New("--meta must contain exactly eight hexadecimal digits")
	}
	decoded, err := hex.DecodeString(value)
	if err != nil {
		return metadata, fmt.Errorf("--meta must contain exactly eight hexadecimal digits: %w", err)
	}
	copy(metadata[:], decoded)
	return metadata, nil
}

func runFormat(path string, sizeMiB uint64, sizeSet bool, out io.Writer) (err error) {
	_, statErr := os.Stat(path)
	var v *volume
	if statErr == nil {
		if sizeSet {
			return errors.New("--size is only valid when creating a new image")
		}
		v, err = openExisting(path)
	} else if errors.Is(statErr, os.ErrNotExist) {
		if !sizeSet {
			return errors.New("--size is required when creating a new image")
		}
		v, err = createImage(path, sizeMiB)
	} else {
		return fmt.Errorf("stat target %s: %w", path, statErr)
	}
	if err != nil {
		return err
	}
	succeeded := false
	defer func() {
		closeErr := v.close()
		if err == nil && closeErr != nil {
			err = fmt.Errorf("close target %s: %w", path, closeErr)
		}
		if v.created && !succeeded {
			_ = os.Remove(path)
		}
	}()
	if err = v.writeEmptyRoot(); err != nil {
		return err
	}
	if err = v.sync(); err != nil {
		return err
	}
	succeeded = true
	_, err = fmt.Fprintf(out, "formatted %s: %d sectors (%d bytes)\n", path, v.sectors, uint64(v.sectors)*sectorSize)
	return err
}

func keysEqual(e entry, key fileKey) bool {
	return e.modeID == key.modeID && e.typeID == key.typeID && e.name == key.name
}

func keyDisplay(key fileKey) string {
	return strings.TrimRight(string(key.name[:]), "\x00")
}
