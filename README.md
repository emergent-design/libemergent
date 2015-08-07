libemergent
=========

A collection of useful classes:

  * [Buffer<>](https://github.com/emergent-design/libemergent/wiki#buffer)
  * [Image<>](https://github.com/emergent-design/libemergent/wiki#image)
  * [Redis](https://github.com/emergent-design/libemergent/wiki#redis)
  * [ResetEvent](https://github.com/emergent-design/libemergent/wiki#resetevent)
  * [DateTime](https://github.com/emergent-design/libemergent/wiki#datetime)
  * [Logger](https://github.com/emergent-design/libemergent/wiki#logger)
  * [Path](https://github.com/emergent-design/libemergent/wiki#path)
  * [Maths](https://github.com/emergent-design/libemergent/wiki#maths)
  * [Reverse](https://github.com/emergent-design/libemergent/wiki#reverse)
  * [String](https://github.com/emergent-design/libemergent/wiki#string)
  * [Type](https://github.com/emergent-design/libemergent/wiki#type)
  * [Uuid](https://github.com/emergent-design/libemergent/wiki#uuid)
  * [Profile](https://github.com/emergent-design/libemergent/wiki#profile)


Installation
============

This is a header-only library, simply ensure that the headers are on the appropriate include path.
Under Ubuntu there are dev packages [available to download](http://downloads.emergent-design.co.uk/libemergent/)
which will install the headers to ```/usr/include```.


Dependencies
============

We rely on [FreeImage](http://freeimage.sourceforge.net/download.html) for loading and saving images. If you
use the image classes then please ensure that FreeImage is available on your system.


Linux
-----

```bash
$ sudo apt-get install build-essential libfreeimage-dev
```

When linking your application remember to include ```-lfreeimage```.


Windows
-------

Download the [FreeImage](http://freeimage.sourceforge.net/download.html) Windows binary image.
  * Copy the header file to the include path of your project.
  * Copy the appropriate dll to the lib path of your project.
  * Ensure that your project links to the library.
