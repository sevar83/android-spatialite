RT Topology Library
===================

## Build status

| OSGeo | GitLab |
|:---   |:---    |
| [![Build Status](https://dronie.osgeo.org/api/badges/rttopo/librttopo/status.svg?branch=master)] (https://dronie.osgeo.org/rttopo/librttopo?branch=master) | [![Gitlab-CI](https://gitlab.com/rttopo/rttopo/badges/master/build.svg)] (https://gitlab.com/rttopo/rttopo/commits/master) |


## About

The RT Topology Library exposes an API to create and manage standard
(ISO 13249 aka SQL/MM) topologies using user-provided [data stores]
(doc/DATASTORES.md) and released under the GNU GPL license
(version 2 or later).

The code is derived from [PostGIS](http://postgis.net) liblwgeom
library enhanced to provide thread-safety, have less dependencies
and be independent from PostGIS release cycles.

The RT Topology Library was funded by "Regione Toscana - SITA"
(CIG: 6445512CC1), which also funded many improvements in the
originating liblwgeom.

Official code repository is https://git.osgeo.org/gitea/rttopo/librttopo

A mirror exists at https://gitlab.com/rttopo/rttopo, automatically
updated on every push to the official repository.

Development mailing list:
https://lists.osgeo.org/mailman/listinfo/librttopo-dev

## Building, testing, installing

### Unix

Using Autotools:

    ./autogen.sh # in ${srcdir}, if obtained from GIT
    ${srcdir}/configure # in build dir
    make # build
    make check # test
    make install # or install-strip

### Microsoft Windows

See [separate document](doc/BUILDING-ON-WINDOWS.md)

## Using

The public header for topology support is `librttopo.h`.
The caller has to setup a backend interface (`RTT_BE_IFACE`) implementing
all the required callbacks and will then be able to use the provided
editing functions.

The contract for each callback is fully specified in the header.
The callbacks are as simple as possible while still allowing for
backend-specific optimizations.

The backend interface is an opaque object and callabcks are registered
into it using free functions. This is to allow for modifying the required
set of callbacks between versions of the library without breaking backward
compatibility.
