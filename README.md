# Auto Type
This turns a Raspberry Pi Pico that into a USB keyboard that types out pre-defined data.

The main benefit of using a pico is cost. The pico is cheap and has a massive 2 MB 
of flash storage, meaning you can include a large amount of data without having to wire up
external storage. Data is also compressed before being compiled in and then decompressed
on the fly, allowing for even more text.

Auto Type features two typing modes:

### Default Mode
After being plugged in the pico waits a few seconds then types out everything.
This requires no extra hardware and is the easiest option.

### Controlled Mode
The pico is controlled by a "next" button and an "enter" button which are
connected via GPIO. The "next" button cycles through data sources. Pressing "enter" once
will print the currently selected source, pressing it twice will actually print the data.
Pressing it twice while data is printing will cancel.

This requires wiring up a few extra buttons, but the extra flexibility and functionality
might be useful.

### UART output
Mostly intended for debugging, the pico will print data out to the UART0 instead of acting
as a keyboard. This is intended to be used with a Picoprobe.

# Data Sources
Using Controlled Mode allows printing only specific data sources. Generally data sources
are text files which are then converted using `dataconvert.py` as part of the build process.
These are included in `sources.h` and then into `decode.c`.

## Adding a Source
Adding a source requires modifying two files: `CMakeLists.txt` and `sources.h`

In `CMakeLists.txt` find the comment "SECTION Add Sources". Call `add_data_source()`
using the commented example below for reference. Note that `${CMAKE_CURRENT_LIST_DIR}`
refers to the directory the `CMakeLists.txt` file is in, and `${CMAKE_BINARY_DIR}` refers
to the build directory.

```cmake
add_data_source(
    # Path to the source file
    INPUT ${CMAKE_CURRENT_LIST_DIR}/sources/example1.txt
    # Path to output files (will generate example1_bin.h and example1_bin.c)
    OUTPUT ${CMAKE_BINARY_DIR}/example1_bin
    # Name of the C array (this will also #define EXAMPLE1_ARRAY_SIZE)
    NAME EXAMPLE1_ARRAY
    # The name of the executable target, should always be "auto_type"
    TARGETS auto_type)
```

Once this is done edit `sources.h`. Update `SOURCES_SIZE` to the new size of the array
and add an entry `{ <NAME>_SIZE, <NAME>, "A short description" },` where `<NAME>` is replaced
with the NAME argument you gave in `CMakeLists.txt`. For example if in the CMake file I put
`NAME BIG_FILE` then my `sources.h` entry might read `{ BIG_FILE_SIZE, BIG_FILE, "A really big file" },`.

