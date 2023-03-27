# FAT for KTANE

Custom file system for SD cards to permit ease of access.

* Designed for the index to be parsed once.
  * Modules should register with the FAT what files they need.
  * FAT scans the index.
  * Each registered file will be returned a block start, block length, final block length.
* Files are always contigous.

## Index

The index starts at block 0 of the SD card, it continues until an End Of Index (EOI) marker is found.

* FAT entries are repeated, once an EOI marker is writen zeros are written until the end of the block.
  * 8bits  - Module Type 0x00 (Unknown Module ID, never has resources), 0xff (All Module Resources)
  * 8bits  - File Index (256 files for a module.)
  * 4bits  - File Type (Up to 16 file types.)
  * 4bits  - Unused
  * 8bits  - Unused  
  * 16bits - Block Size, including last incomplete block. (Up to 65535 blocks, 16Mb files)
  * 16bits - Last Block Usage (Number of useful bytes in last block)  

An EOI marker is a FAT entry of all zeros, though only the File Type is actually checked to be 0x0.

## File Types

 * 0x0 - End of Index
 * 0x1 - Audio (12kHz / 8-bit / mono), all samples must use full block, i.e. 24fps audio. Max audio length at 65k blocks is ~= 45 minutes.