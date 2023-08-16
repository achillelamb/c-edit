# c-edit

An elementary terminal-based text editor written in C
> Special thanks to [@antirez](https://github.com/antirez) for providing the helpful guide/tutorial I used to start the development
## Prerequisites
- [make](https://www.gnu.org/software/make/)
- A C compiler
```sh
# on Ubuntu
sudo apt install build-essential
```
```sh
# on Arch Linux
sudo pacman -S base-devel
```
## Build
Move into the main folder and execute
```sh
make
```
Then, the executable is available via `./c-edit`. You can use it as a stand-alone command or you can pass a text file as an argument.
## Commands 
- `Ctrl` + `q`: quit
- `Ctrl` + `s`: save
