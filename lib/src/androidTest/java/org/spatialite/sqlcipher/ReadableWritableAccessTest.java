package org.spatialite.sqlcipher;

import android.content.Context;
import android.database.Cursor;

import org.junit.Test;
import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteOpenHelper;

import java.io.File;

import static junit.framework.Assert.assertEquals;

public class ReadableWritableAccessTest extends BaseTest {

    @Test
    public void readableWriteableAccess() {

        database.close();
        deleteDatabaseFileAndSiblings(DATABASE_NAME);
        String databasesFolderPath = targetContext.getDatabasePath(DATABASE_NAME).getParent();
        File databasesFolder = new File(databasesFolderPath);
        databasesFolder.delete();

        DatabaseHelper databaseHelper = new DatabaseHelper(targetContext);

        SQLiteDatabase writableDatabase = databaseHelper.getWritableDatabase();
        writableDatabase.beginTransaction();
        SQLiteDatabase readableDatabase = databaseHelper.getReadableDatabase();

        Cursor results = readableDatabase.rawQuery("select count(*) from t1", new String[]{});
        results.moveToFirst();
        int rowCount = results.getInt(0);

        results.close();
        writableDatabase.endTransaction();
        readableDatabase.close();
        writableDatabase.close();

        assertEquals(1, rowCount);
    }

    private class DatabaseHelper extends SQLiteOpenHelper {

        public DatabaseHelper(Context context) {
            super(context, DATABASE_NAME, null, 1);
        }

        @Override
        public void onCreate(SQLiteDatabase database) {
            database.execSQL("create table t1(a,b)");
            database.execSQL("insert into t1(a,b) values(?, ?)", new Object[]{"one for the money",
                                                                              "two for the show"});
        }

        @Override
        public void onUpgrade(SQLiteDatabase database, int oldVersion, int newVersion) {}
    }
}
