package org.spatialite.sqlcipher;

import android.content.Context;

import org.junit.Test;
import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteOpenHelper;

import static junit.framework.Assert.assertTrue;

public class VerifyOnUpgradeIsCalledTest extends BaseTest {

    @Test
    public void verifyOnUpgradeIsCalled() {

        deleteDatabaseFileAndSiblings(DATABASE_NAME);
        DatabaseHelper firstRun = new DatabaseHelper(targetContext, 1);
        SQLiteDatabase db = firstRun.getWritableDatabase();
        db.close();
        DatabaseHelper secondRun = new DatabaseHelper(targetContext, 2);
        SQLiteDatabase db2 = secondRun.getWritableDatabase();
        db2.close();
        assertTrue(secondRun.OnUpgradeCalled);
    }

    class DatabaseHelper extends SQLiteOpenHelper {

        public boolean OnUpgradeCalled;

        public DatabaseHelper(Context context, int version) {
            super(context, DATABASE_NAME, null, version);
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
