package org.spatialite.sqlcipher;

import android.database.Cursor;

import org.junit.Test;

import static junit.framework.Assert.assertEquals;

public class GetTypeFromCrossProcessCursorWrapperTest extends BaseTest {

    @Test
    public void getTypeFromCrossProcessCursorWrapper() {
        database.execSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?, ?);", new Object[]{"one for the money", "two for the show"});
        Cursor cursor = database.query("t1", new String[]{"a", "b"}, null, null, null, null, null);
        cursor.moveToFirst();
        int type_a = cursor.getType(0);
        int type_b = cursor.getType(1);
        cursor.close();
        database.close();
        assertEquals(Cursor.FIELD_TYPE_STRING, type_a);
        assertEquals(Cursor.FIELD_TYPE_STRING, type_b);
    }
}
