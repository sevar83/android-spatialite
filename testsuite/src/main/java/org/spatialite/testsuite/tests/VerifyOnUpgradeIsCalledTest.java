package org.spatialite.testsuite.tests;

import android.content.Context;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteOpenHelper;
import org.spatialite.testsuite.SpatialiteApplication;

public class VerifyOnUpgradeIsCalledTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {

        SpatialiteApplication.getInstance().deleteDatabaseFileAndSiblings(SpatialiteApplication.DATABASE_NAME);
        DatabaseHelper firstRun = new DatabaseHelper(SpatialiteApplication.getInstance(), 1);
        SQLiteDatabase db = firstRun.getWritableDatabase(SpatialiteApplication.DATABASE_PASSWORD);
        db.close();
        DatabaseHelper secondRun = new DatabaseHelper(SpatialiteApplication.getInstance(), 2);
        SQLiteDatabase db2 = secondRun.getWritableDatabase(SpatialiteApplication.DATABASE_PASSWORD);
        db2.close();
        return secondRun.OnUpgradeCalled;
    }

    @Override
    public String getName() {
        return "Verify onUpgrade Is Called Test";
    }

    class DatabaseHelper extends SQLiteOpenHelper {

        public boolean OnUpgradeCalled;

        public DatabaseHelper(Context context, int version) {
            super(context, SpatialiteApplication.DATABASE_NAME, null, version);
        }

        @Override
        public void onCreate(SQLiteDatabase database) {
            database.execSQL("create table t1(a,b)");
        }

        @Override
        public void onUpgrade(SQLiteDatabase database, int oldVersion, int newVersion) {
            OnUpgradeCalled = true;
        }
    }
}
