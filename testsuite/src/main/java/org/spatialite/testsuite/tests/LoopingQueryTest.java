package org.spatialite.testsuite.tests;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;

public class LoopingQueryTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {
        int counter = 0;
        int iterations = 1000;
        database.execSQL("create table t1(a);");
        database.execSQL("insert into t1(a) values (?)", new Object[]{"foo"});
        StringBuilder buffer = new StringBuilder();
        while(counter < iterations){
            Cursor cursor = database.rawQuery("select * from t1", null);
            if(cursor != null){
                cursor.moveToFirst();
                buffer.append(cursor.getString(0));
                cursor.close();
            }
            counter++;
        }
        return buffer.toString().length() > 0;
    }

    @Override
    public String getName() {
        return "Looping Query Test";
    }
}
