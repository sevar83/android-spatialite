package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseHook;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class MigrationUserVersion extends SQLCipherTest {

    @Override
    protected SQLiteDatabase createDatabase(File databasePath) {
        SQLiteDatabaseHook hook = new SQLiteDatabaseHook() {
            public void preKey(SQLiteDatabase database) {}
            public void postKey(SQLiteDatabase database) {
                database.execSQL("PRAGMA cipher_default_kdf_iter = 4000;");
            }
        };
        return SQLiteDatabase.openOrCreateDatabase(databasePath,
                SpatialiteApplication.DATABASE_PASSWORD, null, hook);
    }

    @Override
    public boolean execute(SQLiteDatabase database) {

        try {
            String password = SpatialiteApplication.DATABASE_PASSWORD;
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory(SpatialiteApplication.ONE_X_USER_VERSION_DATABASE);
            File sourceDatabase = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.ONE_X_USER_VERSION_DATABASE);
            SQLiteDatabase originalDatabase = SQLiteDatabase.openOrCreateDatabase(sourceDatabase, password, null, new SQLiteDatabaseHook() {
                public void preKey(SQLiteDatabase database) {
                }
                public void postKey(SQLiteDatabase database) {
                    database.rawExecSQL("PRAGMA cipher_migrate;");
                }
            });
            boolean status = originalDatabase.getVersion() > 0;
            originalDatabase.close();
            return status;

        } catch (Exception e) {
            return false;
        }
    }

    @Override
    protected void tearDown(SQLiteDatabase database) {
        database.execSQL("PRAGMA cipher_default_kdf_iter = 64000;");
    }

    @Override
    public String getName() {
        return "Migrate Database 1.x to Current with user_version";
    }
}
