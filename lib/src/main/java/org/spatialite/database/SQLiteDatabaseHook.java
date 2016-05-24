package org.spatialite.database;

public interface SQLiteDatabaseHook {
    void preKey(SQLiteDatabase database);
    void postKey(SQLiteDatabase database);
}
