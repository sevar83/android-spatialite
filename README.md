[![Release](https://jitpack.io/v/org.bitbucket.buildware/android-spatialite.svg)](https://jitpack.io/#org.bitbucket.buildware/android-spatialite)

USAGE

1) ./gradlew install
2) cd ~/.m2
3) manually rename all directories having 'lib' to 'android-spatialite'
4) edit all xml files by replacing all occurences of 'lib' with 'android-spatialite'
5) in the project build.gradle add:
    repositories {
        mavenLocal()
    }
6) in the app module build.gradle add:
    compile 'org.bitbucket.buildware:android-spatialite:1.0.3@aar'

RELEASE

1) increase the version in the library's module build.gradle like
    version = '1.0.4'
2) add files to git
3) git commmit -m "message"
4) git push origin master
5) git tag 1.0.4
6) git push origin 1.0.4