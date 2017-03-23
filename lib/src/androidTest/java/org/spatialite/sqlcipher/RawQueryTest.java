package org.spatialite.sqlcipher;

import android.database.Cursor;

import org.junit.Test;

import static junit.framework.Assert.assertTrue;

public class RawQueryTest extends BaseTest {

    @Test
    public void rawQueryTest() {
        int rows = 0;
        database.execSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?, ?);",
                new Object[]{"one for the money", "two for the show"});
        Cursor cursor = database.rawQuery("select * from t1;", null);
        if(cursor != null){
            while(cursor.moveToNext()) {
                cursor.getString(0);
                rows++;
            }
            cursor.close();
        }
        assertTrue(rows > 0);
    }
}
