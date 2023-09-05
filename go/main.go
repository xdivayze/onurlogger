package main

import (
	"bufio"
	"embed"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"sync"
)

var deps embed.FS

func main() {
	_, wdir, _, _ := runtime.Caller(0)

	emptyChar := 0x2800

	cmd := exec.Command(fmt.Sprintf("%s/deps/%c", filepath.Dir(wdir), emptyChar))
	stdout, err := cmd.StdoutPipe()

	if err != nil {
		fmt.Fprintf(os.Stderr, "error piping process stdout: %s\n", err)
		os.Exit(-1)
	}

	if err := cmd.Start(); err != nil {
		fmt.Fprintf(os.Stderr, "error starting process: %s\n", err)
		os.Exit(-1)
	}

	var wg sync.WaitGroup

	scanner := bufio.NewScanner(stdout)

	out := make(chan string)
	done := make(chan bool)

	wg.Add(1)
	go func(out chan<- string, done chan<- bool, wg *sync.WaitGroup) {
		for scanner.Scan() {
			text := scanner.Text()
			out <- text
		}
		done <- true
		wg.Done()
	}(out, done, &wg)

	wg.Add(1)
	go func(done chan bool, wg *sync.WaitGroup) {
		for {
			select {
			case _ = <-done:
				wg.Done()
			case data, _ := <-out:
				fmt.Fprintf(os.Stdout, "new data received: %s\n", data)
			}
		}
	}(done, &wg)

	wg.Wait()

}
