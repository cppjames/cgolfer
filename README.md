# C Golfer
**C Golfer** is a program that takes in sets of inputs and outputs and finds the shortest possible C source that produces the outputs on `stdout` when the inputs are passed to it in `stdin`.

Here is an example:
```bash
$ cgolfer -t "" "x"
main(){puts("x");}
```

The `-t` option adds a new test case and is followed by the input and the output. In this case, the program takes no input and it should output the letter `x`. The shortest possible C program that fulfills the requirement is `main(){puts("x");}`.

## How does it work?
C's syntax is very complex and it is very difficult to engineer backwards a program's syntax tree only based on the output it produces. This is why C Golfer takes an elegant brute force approach and tries every possible combination of characters from C's source character set until it finds a version that compiles correctly. Then, the resulting binary is tested and, if it passes all the tests, it is considered correct and the search stops.

## Command line options
- `-n <number>` - Maximum number of characters. C Golfer will only look for programs that have no more than this number of characters. Example: `cgolfer -n 5 -t "" ""` (find the shortest program that does not output anything and that has at most 5 characters). The only answer is `main;`.

- `-s <source>` - Provide a starting point for the generated source (as opposed to starting from no characters and gradually adding more in the order that is specified in `cgolfer.c`)

- `-t <input> <output>` - Adds a new test case with input `<input>` and output `<output>`. Any number of test cases can be added, and they all must be passed for the program to be considered correct. Example: `cgolfer -t a b -t "12 3" "23 4"` (find the shortest program that when given the string `a` produces the output `b` and when given `12 3` outputs `23 4`). Note that without any tests C Golfer will see any program that compiles as correct, so the resulting program will always be `main;`, unless the maximum number of characters is set to be less than 5, or a starting point is provided.

- `-v` - Verbose mode (print all sources as they are being compiled and tested).

- `-w <seconds>` - Set the maximum waiting time. If the generated program takes more than this number of seconds to execute, it is considered that the program does not pass the test and is discarded. Default is 3 seconds. 
