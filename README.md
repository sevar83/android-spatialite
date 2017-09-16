[![Release](https://jitpack.io/v/com.github.sevar83/android-spatialite.svg)](https://jitpack.io/v/com.github.sevar83/android-spatialite.svg)

# android-spatialite

## WHAT IS THIS?
- The [Spatialite](https://www.gaia-gis.it/gaia-sins/) database ported for *Android*
- 100% offline, portable and self-contained as *SQLite*.

## WHEN DO I NEED IT?
- When you need deployment, collecting, processing and fast querying of small to huge amounts of geometry data (points, polylines, polygons, multipolygons, etc.) on Android devices.
- When you want to be 100% independent from any server/cloud backend.

## GETTING STARTED

If you know basic *SQLite*, there's almost nothing to learn. The API is 99% the same as the Android *SQLite* API (as of API level 15). The main difference is the packaging. Use `org.spatialite.database.XYZ` instead of `android.database.sqlite.XYZ` and `org.spatialite.XYZ` instead of `android.database.XYZ`. Same applies to the other classes - all platform `SQLiteXYZ` classes have their *Spatialite* versions.

### Gradle
1) Have this in your project's `build.gradle`:

```
allprojects {
  repositories {
    ...
    maven { url "https://jitpack.io" }
  }
}
```

2) Add the following to your module's `build.gradle`:
```
compile 'com.github.sevar83:android-spatialite:2.0.0'
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
No. It uses cursors - the suggested lightweight approach to access SQL used in the Android platform instead of the heavier JDBC.

### 64-bit architectures supported?

Yes. It builds for `arm64-v8a` and `x86_64`. `mips64` is not tested.

### Reducing the APK size. 

This library is distributed as multi-architecture AAR file. 
By default Gradle will produce a universal APK including the native .so libraries compiled for all supported CPU architectures. Usually that's unacceptable for large libraries like this.
But that's easily fixed by using Gradle's "ABI splits" feature. The following gradle code will produce a separate APK per each architecture. The APK size is reduced few times.
```
android {
    splits {
        abi {
            enable true
                reset()
                include "armeabi-v7a", "arm64-v8a", "x86", "x86_64"
            }
        }
    }
}
```

### What libraries are packaged currently?

- SQLite 3.15.1
- Spatialite 4.3.0a
- GEOS 3.4.2
- Proj4 4.8.0
- lzma 5.2.1
- iconv 1.13
- xml2 2.9.2
- freexl 1.0.2

## REQUIREMENTS
Min SDK 15

## MIGRATION TO 2.0+

1. Remove calls to `SQLiteDatabase.loadLibs()`. Now it is automatically done.
2. Replace all occasions of `import org.spatialite.Cursor;` with `import android.database.Cursor;`
3. Replace all occasions of `import org.spatialite.database.SQLite***Exception;` with `import android.database.sqlite.SQLite***Exception;`

## CHANGES

### 2.0.0
- Now using the [Requery.io SQLite wrapper](https://github.com/requery/sqlite-android/) instead of SQLCipher's. This results to:
- Android Nougat (25+) supported. The native code no more links to private NDK libraries exception and warning messages similar to `UnsatisfiedLinkError: dlopen failed: library "libandroid_runtime.so" not found` should be no more. For more details: https://developer.android.com/about/versions/nougat/android-7.0-changes.html#ndk;
- Much cleaner codebase derived from a much newer and more mature AOSP SQLite wrapper snapshot;
- Now possible to build with the latest NDK (tested on R14);
- Switched to CLang as the default NDK toolchain;
- 64-bit build targets (arm64-v8a, x86_64);
- `SQLiteDatabase.loadLibs()` initialization call is not required anymore;
- Removed `org.spatialite.Cursor` interface. Used 'android.database.sqlite.Cursor' instead.
- Removed the `SQLiteXyzException` classes. Their AOSP originals are used instead;
- Dropped support for Android localized collation. SQL statements with "COLLATE LOCALIZED" will cause error. This is necessary to reduce the library size and ensure N compatibility;
- Updated SQLite to 3.15.1;
- Updated lzma to 5.2.1;
- Updated FreeXL to 1.0.2;

## CREDITS
The main ideas used here were borrowed from:
- https://github.com/requery/sqlite-android
- https://github.com/sqlcipher/android-database-sqlcipher
- https://github.com/illarionov/android-sqlcipher-spatialite

## LICENSE
Apache License 2.0
