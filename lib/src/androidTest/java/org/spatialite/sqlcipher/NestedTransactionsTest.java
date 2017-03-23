package org.spatialite.sqlcipher;

import org.junit.Test;

import static junit.framework.Assert.assertEquals;


public class NestedTransactionsTest extends BaseTest {

    @Test
    public void nestedTransactions() {
        database.execSQL("begin;");
        database.execSQL("savepoint foo;");
        database.execSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?,?);", new Object[]{"one for the money", "two for the show"});
        database.execSQL("savepoint bar;");
        database.execSQL("insert into t1(a,b) values(?,?);", new Object[]{"three to get ready", "go man go"});
        database.execSQL("rollback transaction to bar;");
        database.execSQL("commit;");
        int count = QueryHelper.singleIntegerValueFromQuery(database, "select count(*) from t1;");
        assertEquals(1, count);
    }
}
