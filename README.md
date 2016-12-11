[![Release](https://jitpack.io/v/org.bitbucket.buildware/android-spatialite.svg)](https://jitpack.io/#org.bitbucket.buildware/android-spatialite)

# android-spatialite
---
## WHAT IS THIS?
- The [Spatialite](https://www.gaia-gis.it/gaia-sins/) database ported for *Android*
- 100% offline, portable and self-contained as *SQLite*.

## WHEN DO I NEED IT?
When you need deployment, collecting, processing and fast querying of small to huge amounts of geometry data (points, polylines, polygons, multipolygons, etc.) on Android devices.
When you want to be 100% independent from any server/cloud backend.

## GETTING STARTED

If you know basic *SQLite*, there's almost nothing to learn. The API is 99% the same as the Android *SQLite* API (as of API level 15). The main difference is the packaging. Use `org.spatialite.database.XYZ` instead of `android.database.sqlite.XYZ` and `org.spatialite.XYZ` instead of `android.database.XYZ`. Same applies to the other classes - all platform `SQLiteXYZ` classes have their *Spatialite* versions.

Add the following to your project's `build.gradle`:
```
allprojects {
		repositories {
			...
			maven { url 'https://jitpack.io' }
		}
	}
```
Add the following to your module's `build.gradle`:
```
compile 'com.github.sevar83:android-spatialite:1.0.7'
```

## EXAMPLE CODE
There is a very simple and useless example in the `app` module. Another example is the [SpatiAtlas](https://github.com/sevar83/SpatiAtlas) experiment.

## HOW IT WORKS?
Works the same way as the platform *SQLite*. It's accessible through `Java/JNI` wrappers around the *Spatialite* C library. 
The *Spatialite* wrappers were derived and adapted from the platform *SQLite* wrappers (the standard Android SQLite API).

## Other FAQ

### What is *Spatialite*?
Simply: *Spatialite* = *SQLite* + advanced geospatial support.<br>
*Spatialite* is a geospatial extension to *SQLite*. It is a set of few libraries written in C to extend *SQLite* with geometry data types and many [SQL functions](http://www.gaia-gis.it/gaia-sins/spatialite-sql-4.3.0.html) above geometry data. For more info: https://www.gaia-gis.it/gaia-sins/

### Does it use JDBC?
No. It uses cross-process cursors - the suggested lightweight approach to access SQL used in the Android platform instead of the heavier JDBC.

### What libraries are packaged currently?

- SQLite 3.8.10.2
- Spatialite 4.3.0a
- GEOS 3.4.2
- Proj4 4.8.0
- lzma 5.1.3alpha
- iconv
- xml2 2.9.2
- freexl 1.0

## REQUIREMENTS
Min SDK 15

## CREDITS
The main ideas used here were borrowed from:
- https://github.com/sqlcipher/android-database-sqlcipher and
- https://github.com/illarionov/android-sqlcipher-spatialite

## LICENSE
Apache License 2.0
