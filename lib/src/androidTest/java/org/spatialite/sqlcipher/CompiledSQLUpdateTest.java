package org.spatialite.sqlcipher;

import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.spatialite.database.SQLiteStatement;

import static junit.framework.Assert.assertEquals;

@RunWith(AndroidJUnit4.class)
public class CompiledSQLUpdateTest extends BaseTest {
    @Test
    public void compiledSQLUpdate() {

        database.execSQL("create table ut1(a text, b integer)");
        database.execSQL("insert into ut1(a, b) values (?,?)", new Object[]{"s1", new Integer(100)});

        SQLiteStatement st = database.compileStatement("update ut1 set b = 101 where b = 100");
        assertEquals(1, st.executeUpdateDelete());
    }
}
