package org.spatialite.testsuite.tests;

import org.spatialite.Cursor;
import org.spatialite.database.SQLiteDatabase;

public class GetTypeFromCrossProcessCursorWrapperTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {
        database.execSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?, ?);", new Object[]{"one for the money", "two for the show"});
        Cursor cursor = database.query("t1", new String[]{"a", "b"}, null, null, null, null, null);
        cursor.moveToFirst();
        int type_a = cursor.getType(0);
        int type_b = cursor.getType(1);
        cursor.close();
        database.close();
        return (type_a == Cursor.FIELD_TYPE_STRING) && (type_b == Cursor.FIELD_TYPE_STRING);
    }

    @Override
    public String getName() {
        return "Get Type from CrossProcessCursorWrapper";
    }
}
