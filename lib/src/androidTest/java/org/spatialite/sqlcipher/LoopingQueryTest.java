package org.spatialite.sqlcipher;

import android.database.Cursor;

import org.junit.Test;

import static junit.framework.Assert.assertTrue;

public class LoopingQueryTest extends BaseTest {

    @Test
    public void loopingQuery() {
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
        assertTrue(buffer.toString().length() > 0);
    }
}
