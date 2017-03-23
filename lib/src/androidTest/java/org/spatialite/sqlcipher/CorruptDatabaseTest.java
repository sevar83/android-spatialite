package org.spatialite.sqlcipher;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabaseCorruptException;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.spatialite.database.SQLiteDatabase;

import java.io.File;
import java.io.IOException;

import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.fail;

@RunWith(AndroidJUnit4.class)
public class CorruptDatabaseTest extends BaseTest {

    @Test
    public void corruptDatabase() throws IOException {
        File unencryptedDatabase = targetContext.getDatabasePath("corrupt.db");

        try {
            extractAssetToDatabaseDirectory("corrupt.db");

            SQLiteDatabase database = SQLiteDatabase.openOrCreateDatabase(unencryptedDatabase, null);

            // NOTE: database not expected to be null, but double-check:
            assertNotNull("ERROR: got null database object", database);

            // *Should* have been recovered:
            Cursor cursor = database.rawQuery("select * from sqlite_master;", null);
            assertNotNull("NOT EXPECTED: database.rawQuery() returned null cursor", cursor);

            // *Should* corrupt the database file that is already open:
            extractAssetToDatabaseDirectory("corrupt.db");

            try {
                // Attempt to write to corrupt database file *should* fail:
                database.execSQL("CREATE TABLE t1(a,b);");

                // NOT EXPECTED to get here:
                fail("NOT EXPECTED: CREATE TABLE succeeded ");
            } catch (SQLiteDatabaseCorruptException ex) {
                // Caught SQLiteDatabaseCorruptException as expected OK
            }

            // *Expected* to be closed now
            if (database.isOpen()) {
                fail("NOT EXPECTED: database is still open");
            }
        }
        finally {
            unencryptedDatabase.delete();
        }
    }
}
