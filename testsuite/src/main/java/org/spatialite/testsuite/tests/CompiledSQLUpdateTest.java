package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteStatement;

public class CompiledSQLUpdateTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {

        database.rawExecSQL("create table ut1(a text, b integer)");
        database.execSQL("insert into ut1(a, b) values (?,?)", new Object[]{"s1", new Integer(100)});

        SQLiteStatement st = database.compileStatement("update ut1 set b = 101 where b = 100");
        long recs = st.executeUpdateDelete();
        return (recs == 1);
    }

    @Override
    public String getName() {
        return "Compiled SQL update test";
    }
}
