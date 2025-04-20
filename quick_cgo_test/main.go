package main

// #include "factorial.c"
import "C"
import "fmt"

//export goCallbackHandler
func goCallbackHandler(a, b C.int) C.int {
	fmt.Println("goCallbackHandler")
	return a + b
}

func MyAdd(a, b int) int {
	return int(C.doAdd(C.int(a), C.int(b)))
}

func main() {
	MyAdd(1, 2)
}
