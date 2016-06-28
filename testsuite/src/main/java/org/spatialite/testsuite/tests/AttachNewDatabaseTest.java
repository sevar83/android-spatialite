package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class AttachNewDatabaseTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase encryptedDatabase) {

        encryptedDatabase.execSQL("create table t1(a,b)");
        encryptedDatabase.execSQL("insert into t1(a,b) values(?, ?)", new Object[]{"one", "two"});

        String newKey = "foo";
        File newDatabasePath = SpatialiteApplication.getInstance().getDatabasePath("normal.db");
        String attachCommand = "ATTACH DATABASE ? as encrypted KEY ?";
        String createCommand = "create table encrypted.t1(a,b)";
        String insertCommand = "insert into encrypted.t1 SELECT * from t1";
        String detachCommand = "DETACH DATABASE encrypted";
        encryptedDatabase.execSQL(attachCommand, new Object[]{newDatabasePath.getAbsolutePath(), newKey});
        encryptedDatabase.execSQL(createCommand);
        encryptedDatabase.execSQL(insertCommand);
        encryptedDatabase.execSQL(detachCommand);

        return true;
    }

    @Override
    protected void tearDown(SQLiteDatabase database) {
        File newDatabasePath = SpatialiteApplication.getInstance().getDatabasePath("normal.db");
        newDatabasePath.delete();
    }

    public android.database.sqlite.SQLiteDatabase createNormalDatabase(){
        File newDatabasePath = SpatialiteApplication.getInstance().getDatabasePath("normal.db");
        return android.database.sqlite.SQLiteDatabase.openOrCreateDatabase(newDatabasePath.getAbsolutePath(), null);
    }

    @Override
    public String getName() {
        return "Attach New Database Test";
    }
}
