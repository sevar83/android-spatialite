package org.spatialite.testsuite.tests;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseHook;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class MigrateDatabaseFrom1xFormatToCurrentFormat extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {

        try {
            String password = SpatialiteApplication.DATABASE_PASSWORD;
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory(SpatialiteApplication.ONE_X_DATABASE);
            File sourceDatabase = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.ONE_X_DATABASE);
            SQLiteDatabaseHook hook = new SQLiteDatabaseHook() {
                public void preKey(SQLiteDatabase database) {}
                public void postKey(SQLiteDatabase database) {
                    database.rawExecSQL("PRAGMA cipher_migrate;");
                }
            };
            SQLiteDatabase source = SQLiteDatabase.openOrCreateDatabase(sourceDatabase.getPath(), null, hook);
            Cursor result = source.rawQuery("select * from t1", new String[]{});
            if(result != null){
                result.moveToFirst();
                String a = result.getString(0);
                String b = result.getString(1);
                result.close();
                source.close();
                return a.equals("one for the money") &&
                       b.equals("two for the show");
            }
            return false;
            
        } catch (Exception e) {
            return false;
        }
    }

    @Override
    public String getName() {
        return "Migrate Database 1.x to Current Test";
    }
}
