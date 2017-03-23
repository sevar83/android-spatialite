package org.spatialite.sqlcipher;

import org.junit.Test;
import org.spatialite.database.SQLiteDatabase;

import static junit.framework.Assert.assertTrue;

public class OpenReadOnlyDatabaseTest extends BaseTest {

    @Test
    public void openReadOnlyDatabase() {
        database.close();
        database = SQLiteDatabase.openDatabase(databasePath.getAbsolutePath(), null, SQLiteDatabase.OPEN_READONLY);
        boolean opened = database.isOpen();
        database.close();
        assertTrue(opened);
    }
}
