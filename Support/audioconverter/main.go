package main

import (
	"flag"
	"io"
	"log"
	"os"
)

const SrcSectorSize = 500
const DstSectorSize = 512

func main() {
	sourceFile := flag.String("src", "", "Source raw file to convert.")
	destFile := flag.String("dst", "", "Destination raw file location.")

	flag.Parse()

	src, err := os.Open(*sourceFile)
	if err != nil {
		log.Panicf("opening file failed: %v", err)
	}
	defer src.Close()

	dst, err := os.Create(*destFile)
	if err != nil {
		log.Panicf("opening file failed: %v", err)
	}
	defer dst.Close()

	for {
		padding := make([]byte, DstSectorSize-SrcSectorSize)
		sector := make([]byte, SrcSectorSize)

		if readN, err := src.Read(sector); err != nil {
			if err == io.EOF {
				break
			} else {
				log.Panicf("reading file failed: %v", err)
			}

		} else if readN != SrcSectorSize {
			log.Printf("reading file sector only read: %d", readN)
		}

		if n, err := dst.Write(sector); err != nil || n != len(sector) {
			log.Panicf("write file sector failed: %v/%d", err, n)
		}
		if n, err := dst.Write(padding); err != nil || n != len(padding) {
			log.Panicf("write file padding failed: %v/%d", err, n)
		}
	}

	log.Printf("done")
}
