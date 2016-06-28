package org.spatialite.testsuite.tests;

import android.util.Log;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseHook;
import org.spatialite.testsuite.QueryHelper;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;
import java.util.Date;

public class AverageOpenTimeTest extends SQLCipherTest {

    int MAX_ATTEMPTS = 10;

    SQLiteDatabaseHook hook = new SQLiteDatabaseHook() {
        public void preKey(SQLiteDatabase sqLiteDatabase) {
        }
        public void postKey(SQLiteDatabase sqLiteDatabase) {
            //sqLiteDatabase.rawExecSQL("PRAGMA kdf_iter = 64000;");
        }
    };

    @Override
    public boolean execute(SQLiteDatabase database) {

        database.close();
        boolean status = false;
        Float runs = 0.0f;
        File databasePath = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);
        for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
            Date start = new Date();
            database = SQLiteDatabase.openOrCreateDatabase(databasePath, SpatialiteApplication.DATABASE_PASSWORD, null, hook);
            Date end = new Date();
            String kdf = QueryHelper.singleValueFromQuery(database, "PRAGMA kdf_iter;");
            Log.i(TAG, String.format("KDF value:%s", kdf));
            database.close();
            runs += (end.getTime() - start.getTime()) / 1000.0f;
        }

        status = true;
        setMessage(String.format("%s attempts, %.3f second average open time", MAX_ATTEMPTS, runs/MAX_ATTEMPTS));
        return status;
    }

    @Override
    protected SQLiteDatabase createDatabase(File databasePath) {
        return SQLiteDatabase.openOrCreateDatabase(databasePath, SpatialiteApplication.DATABASE_PASSWORD, null, hook);
    }

    @Override
    public String getName() {
        return "Average Database Open Time";
    }
}
