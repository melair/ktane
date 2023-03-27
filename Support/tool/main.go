package main

import (
	"github.com/melair/ktane/cmd"
	_ "github.com/melair/ktane/cmd/sdcard"
	_ "github.com/melair/ktane/cmd/sdcard/build"
)

func main() {
	cmd.Execute()
}
