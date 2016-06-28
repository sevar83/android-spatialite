package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.QueryHelper;


public class NestedTransactionsTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {
        database.rawExecSQL("savepoint foo;");
        database.rawExecSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?,?);", new Object[]{"one for the money", "two for the show"});
        database.rawExecSQL("savepoint bar;");
        database.execSQL("insert into t1(a,b) values(?,?);", new Object[]{"three to get ready", "go man go"});
        database.rawExecSQL("rollback transaction to bar;");
        database.rawExecSQL("commit;");
        int count = QueryHelper.singleIntegerValueFromQuery(database, "select count(*) from t1;");
        return count == 1;
    }

    @Override
    public String getName() {
        return "Nested Transactions Test";
    }
}
