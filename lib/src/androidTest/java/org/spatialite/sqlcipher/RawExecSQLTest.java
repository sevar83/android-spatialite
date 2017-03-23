package org.spatialite.sqlcipher;

import android.database.Cursor;

import org.junit.Test;

import static junit.framework.Assert.assertEquals;

public class RawExecSQLTest extends BaseTest {
    @Test
    public void execSqlTest() {

        String actual = "";
        String value = "hey";
        database.execSQL("create table t1(a)");
        database.execSQL("insert into t1(a) values (?)", new Object[]{value});
        Cursor result = database.rawQuery("select * from t1", new String[]{});
        if(result != null){
            result.moveToFirst();
            actual = result.getString(0);
            result.close();
        }
        assertEquals(value, actual);
    }
}
