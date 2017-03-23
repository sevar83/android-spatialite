package org.spatialite.sqlcipher;

import android.database.Cursor;

import org.junit.Test;

import static junit.framework.Assert.assertTrue;

public class NullQueryResultTest extends BaseTest {

    @Test
    public void nullQuery() {
        
        database.execSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values (?, ?)", new Object[]{"foo", null});
        database.execSQL("insert into t1(a,b) values (?, ?)", new Object[]{"bar", null});
        Cursor cursor = database.rawQuery("select a from t1", null);
        StringBuilder buffer = new StringBuilder();
        while(cursor.moveToNext()){
            buffer.append(cursor.getString(0));
        }
        cursor.close();
        assertTrue(buffer.toString().length() > 0);
    }
}
