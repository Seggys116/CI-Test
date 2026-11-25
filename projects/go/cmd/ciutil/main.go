package main

import (
	"os"

	"ciutil/ciutil"
)

func main() {
	os.Exit(ciutil.Run(os.Args[1:], os.Stdout, os.Stderr))
}
