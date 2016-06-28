package org.spatialite.testsuite.tests;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;

public class RawExecSQLTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {

        String actual = "";
        String value = "hey";
        database.rawExecSQL("create table t1(a)");
        database.execSQL("insert into t1(a) values (?)", new Object[]{value});
        Cursor result = database.rawQuery("select * from t1", new String[]{});
        if(result != null){
            result.moveToFirst();
            actual = result.getString(0);
            result.close();
        }
        return actual.equals(value);
    }

    @Override
    public String getName() {
        return "rawExecSQL Test";
    }
}
