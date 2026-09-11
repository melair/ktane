package main

import (
	"fmt"
	"os"

	"github.com/melair/ktane/Firmware/Tool/cmd"
)

func main() {
	if err := cmd.Execute(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
