package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.QueryHelper;

public class QueryIntegerToStringTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {
        String value = QueryHelper.singleValueFromQuery(database, "SELECT 123;");
        return value.equals("123");
    }

    @Override
    public String getName() {
        return "Query Integer to String";
    }
}
