package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.QueryHelper;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class RawRekeyTest extends SQLCipherTest {

    String password = "x\'2DD29CA851E7B56E4697B0E1F08507293D761A05CE4D1B628663F411A8086D99\'";
    String rekeyCommand = String.format("PRAGMA rekey  = \"%s\";", password);
    File databaseFile = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);

    @Override
    public boolean execute(SQLiteDatabase database) {
        database.execSQL("create table t1(a,b);");
        database.execSQL("insert into t1(a,b) values(?,?)", new Object[]{"one for the money", "two for the show"});
        database.rawExecSQL(rekeyCommand);
        database.close();
        database = SQLiteDatabase.openOrCreateDatabase(databaseFile, password, null);
        int count = QueryHelper.singleIntegerValueFromQuery(database, "select count(*) from t1;");
        boolean status = count == 1;
        database.close();
        return status;
    }

    @Override
    public String getName() {
        return "Raw Rekey Test";
    }
}
