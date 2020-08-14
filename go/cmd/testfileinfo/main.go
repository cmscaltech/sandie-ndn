package main

import (
	"fmt"
	"os"
)

func main() {
	fmt.Println("Hello, World!")
	fi, e := os.Stat("/Users/cataliniordache/Desktop/test.png")
	if e != nil {
		fmt.Println(e)
		os.Exit(1)
	}

	fmt.Println(fi)
	fmt.Println(fi.Size())
	fmt.Println(fi.Mode())

}
