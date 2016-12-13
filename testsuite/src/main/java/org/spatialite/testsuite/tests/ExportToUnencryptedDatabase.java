package org.spatialite.testsuite.tests;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseHook;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class ExportToUnencryptedDatabase extends SQLCipherTest {

    File unencryptedFile;

    @Override
    public boolean execute(SQLiteDatabase database) {

        database.close();
        SpatialiteApplication.getInstance().deleteDatabase(SpatialiteApplication.DATABASE_NAME);
        File databaseFile = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);
        SQLiteDatabaseHook hook = new SQLiteDatabaseHook() {
            public void preKey(SQLiteDatabase sqLiteDatabase) {
                sqLiteDatabase.rawExecSQL("PRAGMA cipher_default_use_hmac = off;");
            }
            public void postKey(SQLiteDatabase sqLiteDatabase) {}
        };
        database = SQLiteDatabase.openOrCreateDatabase(databaseFile, null, hook);

        database.rawExecSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?, ?);", new Object[]{"one for the money", "two for the show"});
        unencryptedFile = SpatialiteApplication.getInstance().getDatabasePath("plaintext.db");
        database.rawExecSQL(String.format("ATTACH DATABASE '%s' as plaintext KEY '';",
                unencryptedFile.getAbsolutePath()));
        database.rawExecSQL("SELECT sqlcipher_export('plaintext');");
        database.rawExecSQL("DETACH DATABASE plaintext;");

        SQLiteDatabase unencryptedDatabase = SQLiteDatabase.openOrCreateDatabase(unencryptedFile, null, hook);
        Cursor cursor = unencryptedDatabase.rawQuery("select * from t1;", new String[]{});
        String a = "";
        String b = "";
        while(cursor.moveToNext()){
            a = cursor.getString(0);
            b = cursor.getString(1);
        }
        cursor.close();
        database.close();
        return a.equals("one for the money") &&
               b.equals("two for the show");
    }

    @Override
    protected void tearDown(SQLiteDatabase database) {
        unencryptedFile.delete();
    }

    @Override
    public String getName() {
        return "Export to Unencrypted Database";
    }
}
