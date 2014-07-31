libemergent
=========

A collection of useful classes.


Installation
============

Under Ubuntu there are 64-bit packages [available to download](http://downloads.emergent-design.co.uk/libemergent/). For
all other platforms it must be built from source and manually installed.


Building
========

We use [premake4](http://industriousone.com/premake) to generate files for make, but it can also be used to generate
project files for a variety of IDEs such as CodeLite, Code::Blocks and Visual Studio. On Ubuntu it can be installed
via apt-get, otherwise it should be [downloaded](http://industriousone.com/premake/download) and made available on the path.

Linux
-----

```bash
$ sudo apt-get install build-essential libfreeimage-dev
$ premake4 gmake
$ make
```

Windows (mingw)
---------------

  * Install [MinGW](http://www.mingw.org/) and make its "bin" folder available on the path.
  * Download the [FreeImage](http://freeimage.sourceforge.net/download.html) Windows binary image.
    * Copy the header file to `{path-to-mingw}/include`.
    * Copy the dll to `{path-to-mingw}/lib`.

```cmd
> premake4 gmake
> mingw32-make
```

To build a 64-bit library you will need to install the appropriate version of [MinGW](http://mingw-w64.sourceforge.net/) and possibly
build the FreeImage library manually.


Windows (msvc)
--------------

Copy the FreeImage header to the include folder of libemergent.

```cmd
> premake4 vs2010
```

The generated solution can then be opened in Visual Studio, updated to the latest version and then
built.

*Note*: The msvc buid will generate a static library since this avoids polluting the headers with
`__declspec(dllexport)` and `__declspec(dllimport)`.
