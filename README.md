libemergent
=========

A collection of useful classes:

  * [Image<>](https://github.com/emergent-design/libemergent/wiki#image)
  * [ResetEvent](https://github.com/emergent-design/libemergent/wiki#resetevent)
  * [DateTime](https://github.com/emergent-design/libemergent/wiki#datetime)
  * [Logger](https://github.com/emergent-design/libemergent/wiki#logger)
  * [Maths](https://github.com/emergent-design/libemergent/wiki#maths)
  * [Reverse](https://github.com/emergent-design/libemergent/wiki#reverse)
  * [String](https://github.com/emergent-design/libemergent/wiki#string)
  * [Type](https://github.com/emergent-design/libemergent/wiki#type)
  * [Uuid](https://github.com/emergent-design/libemergent/wiki#uuid)
  * [Profile](https://github.com/emergent-design/libemergent/wiki#profile)
  * [ThreadPool](https://github.com/emergent-design/libemergent/wiki#threadpool)
  * [Redis](https://github.com/emergent-design/libemergent/wiki/Redis)
  * [Clap](https://github.com/emergent-design/libemergent/wiki/Clap)

Installation
============

This is a header-only library, simply ensure that the headers are on the appropriate include path.
Under Ubuntu there are dev packages [available to download](https://github.com/emergent-design/libemergent/releases)
which will install the headers to ```/usr/include```.


Dependencies
============

libemergent has the following external dependencies. You must ensure that the appropriate headers and binaries are available on the include/lib paths if you use specific features of this library.

  * [FreeImage](http://freeimage.sourceforge.net/download.html) for loading and saving images. Your application must link to this if you use the Image<> class.
  * [hiredis](https://github.com/redis/hiredis) client library. Your application must link to this if you use any of the Redis classes.

On debian-based systems the relevant dependencies can be easily installed as follows:

```bash
$ sudo apt-get install build-essential libfreeimage-dev libhiredis-dev
```
