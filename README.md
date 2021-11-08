# Saturn Satiator: Menu

This is the menu code for Professor Abrasive's forthcoming
Satiator card. This card will jack in to the MPEG
card slot on a Sega Saturn, and allows you to connect a USB
thumb drive or hard disk. This menu provides a user
interface to allow you to choose boot images, manage save
games, and so forth.

The menu is open source, though the Satiator itself is not.
This means you can extend or replace the interface and
features as you please. The disk access API is also exposed,
so Saturn developers can add native USB drive
access to their software.

## Status

This is release grade, but not feature complete.

Next up:

- There needs to be a save data manager.
- It is UGLY A F
- the Satiator API needs to be a standalone library for other programs to use
- It'd be nice to get rid of the dependency on iapetus.

## Building

To get started:

Linux
```
git clone https://github.com/satiator/satiator-menu.git
cd satiator-menu
git submodule update --init --recursive
make
```

Windows
```
git clone --branch use-toolchain-newlib https://github.com/satiator/satiator-menu.git
cd satiator-menu
git submodule update --init --recursive
build
```

This will configure and build 
[Iapetus](https://github.com/cyberwarriorx/iapetus), newlib (libc), then
the Satiator menu.
You may need to edit the `Makefile` to set your cross
compiler. The default toolchain prefix is `sh-none-elf-`.
