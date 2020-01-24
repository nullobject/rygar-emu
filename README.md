# Rygar

This is an emulator for one of my favourite arcade games from the 1980s, Rygar.

<img alt="Rygar" src="https://raw.githubusercontent.com/nullobject/rygar/master/rygar.png" />

## How to Play

You can [run it in your browser](https://rygar.joshbassett.info).

- UP/DOWN/LEFT/RIGHT: move
- Z: attack
- X: jump
- 5: insert coin
- 1: start

## How to Build

```
$ sudo apt install python build-essential cmake cmake-curses-gui libasound2-dev libgl1-mesa-dev xorg-dev
$ mkdir workspace && cd workspace
$ git clone git@github.com:nullobject/rygar.git && cd rygar
$ ./fips build
$ ./fips run rygar
```
