package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class OpenReadOnlyDatabaseTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {
        database.close();
        File databasePath = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);
        database = SQLiteDatabase.openDatabase(databasePath.getAbsolutePath(), SpatialiteApplication.DATABASE_PASSWORD,
                                                null, SQLiteDatabase.OPEN_READONLY);
        boolean opened = database.isOpen();
        database.close();
        return opened;
    }

    @Override
    public String getName() {
        return "Open Read Only Database Test";
    }
}
