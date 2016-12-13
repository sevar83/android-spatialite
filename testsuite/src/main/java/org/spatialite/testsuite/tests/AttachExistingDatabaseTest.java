package org.spatialite.testsuite.tests;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseHook;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;
import java.io.IOException;

public class AttachExistingDatabaseTest extends SQLCipherTest {

    @Override
    protected SQLiteDatabase createDatabase(File databasePath) {
        SQLiteDatabaseHook hook = new SQLiteDatabaseHook() {
            public void preKey(SQLiteDatabase database) {}
            public void postKey(SQLiteDatabase database) {
                database.execSQL("PRAGMA cipher_default_kdf_iter = 4000;");
            }
        };
        return SQLiteDatabase.openOrCreateDatabase(databasePath, null, hook);
    }

    @Override
    public boolean execute(SQLiteDatabase database) {

        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory(SpatialiteApplication.ONE_X_DATABASE);
            File other = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.ONE_X_DATABASE);
            String otherPath = other.getAbsolutePath();
            String attach = String.format("attach database ? as other key ?");
            database.rawExecSQL("pragma cipher_default_use_hmac = off");
            database.rawExecSQL("pragma cipher_default_kdf_iter = 4000;");
            database.execSQL(attach, new Object[]{otherPath, SpatialiteApplication.DATABASE_PASSWORD});
            Cursor result = database.rawQuery("select * from other.t1", new String[]{});
            String a = "";
            String b = "";
            if(result != null){
                result.moveToFirst();
                a = result.getString(0);
                b = result.getString(1);
                result.close();
            }
            database.execSQL("detach database other");
            database.rawExecSQL("pragma cipher_default_kdf_iter = 64000;");
            return a.length() > 0 && b.length() > 0;
        } catch (IOException e) {
            return false;
        }
    }

    @Override
    protected void tearDown(SQLiteDatabase database) {
        SpatialiteApplication.getInstance().delete1xDatabase();
        database.execSQL("PRAGMA cipher_default_kdf_iter = 64000;");
    }

    @Override
    public String getName() {
        return "Attach Existing Database Test";
    }
}
