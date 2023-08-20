package common

type FATEntry struct {
	Module         uint8  `bcfieldwidth:"8"`
	Index          uint8  `bcfieldwidth:"8"`
	Type           uint8  `bcfieldwidth:"4"`
	Unused1        uint8  `bcfieldwidth:"4"`
	Unused2        uint8  `bcfieldwidth:"8"`
	Size           uint16 `bcfieldwidth:"16" bcendian:"little"`
	LastBlockUsage uint16 `bcfieldwidth:"16" bcendian:"little"`
}
