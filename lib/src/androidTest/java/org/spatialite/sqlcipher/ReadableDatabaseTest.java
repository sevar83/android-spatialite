package org.spatialite.sqlcipher;

import android.content.Context;
import android.database.Cursor;

import org.junit.Test;
import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteOpenHelper;

import java.io.File;

import static junit.framework.Assert.assertTrue;

public class ReadableDatabaseTest extends BaseTest {

    @Test
    public void readableDatabase() {
        File databasesDirectory = new File(databasePath.getParent());
        for(File file : databasesDirectory.listFiles()){
            file.delete();
        }
        ReadableDatabaseHelper helper = new ReadableDatabaseHelper(targetContext);
        SQLiteDatabase readableDatabase = helper.getReadableDatabase();
        Cursor results = readableDatabase.rawQuery("select * from t1", new String[]{});
        int resultCount = 0;
        while (results.moveToNext()){
            resultCount++;
        }
        readableDatabase.close();
        assertTrue(resultCount > 0);
    }

    class ReadableDatabaseHelper extends SQLiteOpenHelper {

        public ReadableDatabaseHelper(Context context) {
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
