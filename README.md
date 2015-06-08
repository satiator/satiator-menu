# Saturn Satisfier: Menu

This project is not yet public. Please keep it under your
hat until I officially open the source. Thank you!

This is the menu code for Professor Abrasive's forthcoming
Saturn Satisfier card. This card will jack in to the MPEG
card slot on a Sega Saturn, and allows you to connect a USB
thumb drive or hard disk. This menu provides a user
interface to allow you to choose boot images, manage save
games, and so forth. Or it will when it's done.

The menu is open source, though the Satisfier itself is not.
This means you can extend or replace the interface and
features as you please. The disk access API is also exposed,
so Saturn developers can add native USB drive
access to their software.

## Status

This is pre-alpha software.

There is a Yabause fork with Satisfier emulation support
[here](https://github.com/abrasive/satisfier-yabause);
this is partially complete. Currently supports the
filesystem API; drive emulation is still to come.
The filesystem API emulation uses some fairly recent POSIX
features and may not work on Windows. Patches welcome.

The menu itself is nonexistent at the time of writing; the
build infrastructure works, and you can make filesystem calls.

## Building

To get started:

```
git clone https://github.com/cyberwarriorx/iapetus.git
git clone https://github.com/abrasive/satisfier.git
cd satisfier
make
```

This will configure and build 
[Iapetus](https://github.com/cyberwarriorx/iapetus), then
the Satisfier menu.
You may need to edit the `Makefile` to set your cross
compiler and the path to your Iapetus checkout. The default
toolchain prefix is `sh-none-elf-`.

## Testing

To boot the menu in Yabause, set the emulator to use
the freshly compiled `out/menu.bin` as the MPEG ROM. You
may need a CD in the emulated drive to boot successfully; at
the time of writing, a clean boot is indicated by a fade to
black from the CD player screen.

You may wish to compile Yabause with `-DSATIS_DEBUG` to get
Satisfier-related messages written to the log (eg. by
configuring with `cmake
-DCMAKE_C_FLAGS:STRING=-DSATIS_DEBUG`).
