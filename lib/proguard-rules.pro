-keepclasseswithmembers class org.spatialite.** {
  native <methods>;
  public <init>(...);
}
-keepnames class org.spatialite.** { *; }
-keep public class org.spatialite.database.SQLiteCustomFunction { *; }
-keep public class org.spatialite.database.SQLiteCursor { *; }
-keep public class org.spatialite.database.SQLiteDebug** { *; }
-keep public class org.spatialite.database.SQLiteDatabase { *; }
-keep public class org.spatialite.database.SQLiteOpenHelper { *; }
-keep public class org.spatialite.database.SQLiteStatement { *; }
-keep public class org.spatialite.CursorWindow { *; }
-keepattributes Exceptions,InnerClasses
