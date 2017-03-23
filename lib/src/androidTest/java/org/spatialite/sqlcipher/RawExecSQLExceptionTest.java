package org.spatialite.sqlcipher;

import org.junit.Test;

public class RawExecSQLExceptionTest extends BaseTest {

    @Test(expected = Exception.class)
    public void execSqlException() {
        database.execSQL("select foo from bar");
    }
}
