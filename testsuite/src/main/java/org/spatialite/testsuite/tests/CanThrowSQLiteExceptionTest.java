package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteException;

public class CanThrowSQLiteExceptionTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {

        try{
            throw new SQLiteException();
        }catch (SQLiteException ex){
            return true;
        }
    }

    @Override
    public String getName() {
        return "SQLiteException Test";
    }
}
