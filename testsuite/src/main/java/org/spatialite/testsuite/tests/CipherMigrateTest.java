package org.spatialite.testsuite.tests;

import android.util.Log;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseHook;
import org.spatialite.testsuite.QueryHelper;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class CipherMigrateTest extends SQLCipherTest {

    File olderFormatDatabase = SpatialiteApplication.getInstance().getDatabasePath("2x.db");

    @Override
    public boolean execute(SQLiteDatabase database) {
        database.close();
        final boolean[] status = {false};
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("2x.db");
            SQLiteDatabaseHook hook = new SQLiteDatabaseHook() {
                public void preKey(SQLiteDatabase database) {}
                public void postKey(SQLiteDatabase database) {
                    String value = QueryHelper.singleValueFromQuery(database, "PRAGMA cipher_migrate");
                    status[0] = Integer.valueOf(value) == 0;
                }
            };
            database = SQLiteDatabase.openOrCreateDatabase(olderFormatDatabase,
                    SpatialiteApplication.DATABASE_PASSWORD, null, hook);
            if(database != null){
                database.close();
            }
        } catch (Exception e) {
            Log.i(TAG, "error", e);
        }
        return status[0];
    }

    @Override
    public String getName() {
        return "Cipher Migrate Test";
    }
}
