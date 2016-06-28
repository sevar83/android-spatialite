package org.spatialite.testsuite.tests;

import android.content.Context;
import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteOpenHelper;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class ReadableDatabaseTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {

        File databaseFile = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);
        File databasesDirectory = new File(databaseFile.getParent());
        for(File file : databasesDirectory.listFiles()){
            file.delete();
        }
        ReadableDatabaseHelper helper = new ReadableDatabaseHelper(SpatialiteApplication.getInstance());
        SQLiteDatabase readableDatabase = helper.getReadableDatabase(SpatialiteApplication.DATABASE_PASSWORD);
        Cursor results = readableDatabase.rawQuery("select * from t1", new String[]{});
        int resultCount = 0;
        while (results.moveToNext()){
            resultCount++;
        }
        readableDatabase.close();
        return resultCount > 0;
    }

    @Override
    public String getName() {
        return "Readable Database Test";
    }

    class ReadableDatabaseHelper extends SQLiteOpenHelper {

        public ReadableDatabaseHelper(Context context) {
            super(context, SpatialiteApplication.DATABASE_NAME, null, 1);
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
