Building on Windows
===================

On Windows systems you can choose between two different compilers:
- MinGW / MSYS
  This represents a smart porting of a minimalistic Linux-like
  development toolkit
- Microsoft Visual Studio .NET (aka MSVC)
  This is the standard platform development toolkit from Microsoft.

# Using MinGW / MSYS

We assume that you have already installed the MinGW compiler and the
MSYS shell.
Building 'librttopo' under Windows is then more or less like building
on any other UNIX-like system. If you have unpacked the sources as
C:\librttpopo-<version>, then the required steps are:

  cd c:/librttopo-<version>
  export "CFLAGS=-I/usr/local/include"
  export "LDFLAGS=-L/usr/local/lib"
  ./configure
  make
  make install
  # or (in order to save some disk space)
  make install-strip


# Using Microsoft Visual Studio .NET

We assume that you have already installed Visual Studio enabling the
command
line tools. Note that you are expected to the Visual Studio command
prompt shell
rather than the GUI build environment. If you have unpacked the
sources as
C:\librttpopo-<version>, then the required steps are:

  cd c:\librttopo-<version>
  nmake /f makefile.vc
  nmake /f makefile.vc install

Please note: the MSVC build configuration is based on the OSGeo4W
distribution. Any depending library (e.g. GEOS) is expected to be
already installed on C:\OSGeo4W

