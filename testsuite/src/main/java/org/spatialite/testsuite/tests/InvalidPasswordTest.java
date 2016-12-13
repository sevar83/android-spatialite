package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class InvalidPasswordTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {

        boolean status = false;
        database.rawExecSQL("create table t1(a,b);");
        database.close();
        File databaseFile = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);
        try{
            SQLiteDatabase.openOrCreateDatabase(databaseFile, null);
        } catch (Exception e){
            try {
                database = SQLiteDatabase.openOrCreateDatabase(databaseFile, null);
                database.execSQL("insert into t1(a,b) values(?, ?)", new Object[]{"testing", "123"});
                status = true;
            } catch (Exception ex){}
        }
        return status;
    }

    @Override
    public String getName() {
        return "Invalid Password Test";
    }
}
