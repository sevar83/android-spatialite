package org.spatialite.testsuite.tests;

import android.content.Context;

import org.spatialite.Cursor;
import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteOpenHelper;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;
import java.util.Random;

public class CursorAccessTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {

        database.close();
        SpatialiteApplication.getInstance().deleteDatabaseFileAndSiblings(SpatialiteApplication.DATABASE_NAME);
        String databasesFolderPath = SpatialiteApplication.getInstance()
                                                   .getDatabasePath(SpatialiteApplication.DATABASE_NAME).getParent();
        File databasesFolder = new File(databasesFolderPath);
        databasesFolder.delete();

        MyHelper databaseHelper = new MyHelper(SpatialiteApplication.getInstance());

        SQLiteDatabase db = databaseHelper.getWritableDatabase(SpatialiteApplication.DATABASE_PASSWORD);

        Cursor results = db.rawQuery("select * from t1", new String[]{});
        results.moveToFirst();
        int type_a = results.getType(0);
        int type_b = results.getType(1);
        int type_c = results.getType(2);
        int type_d = results.getType(3);
        int type_e = results.getType(4);

        results.close();
        db.close();

        return type_a == Cursor.FIELD_TYPE_STRING &&
                type_b == Cursor.FIELD_TYPE_INTEGER &&
                type_c == Cursor.FIELD_TYPE_NULL &&
                type_d == Cursor.FIELD_TYPE_FLOAT &&
                type_e == Cursor.FIELD_TYPE_BLOB;
    }

    @Override
    public String getName() {
        return "Cursor Access Test";
    }

    private class MyHelper extends SQLiteOpenHelper {

        public MyHelper(Context context) {
            super(context, SpatialiteApplication.DATABASE_NAME, null, 1);
        }

        @Override
        public void onCreate(SQLiteDatabase database) {
            database.execSQL("create table t1(a text, b integer, c text, d real, e blob)");
            byte[] data = new byte[10];
            new Random().nextBytes(data);
            database.execSQL("insert into t1(a, b, c, d, e) values(?, ?, ?, ?, ?)", new Object[]{"test1", 100, null, 3.25, data});
        }

        @Override
        public void onUpgrade(SQLiteDatabase database, int oldVersion, int newVersion) {}
    }
}
