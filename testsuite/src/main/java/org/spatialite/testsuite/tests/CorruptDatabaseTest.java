package org.spatialite.testsuite.tests;

import android.database.Cursor;
import android.util.Log;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseCorruptException;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class CorruptDatabaseTest extends SQLCipherTest {

    @Override
    public TestResult run() {

        TestResult result = new TestResult(getName(), false);
        try {
            result.setResult(execute(null));
            SQLiteDatabase.releaseMemory();
        } catch (Exception e) {
            Log.v(SpatialiteApplication.TAG, e.toString());
        }
        return result;
    }

    @Override
    public boolean execute(SQLiteDatabase null_database_ignored) {

        File unencryptedDatabase = SpatialiteApplication.getInstance().getDatabasePath("corrupt.db");

        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            SQLiteDatabase database = SQLiteDatabase.openOrCreateDatabase(unencryptedDatabase, null);

            // NOTE: database not expected to be null, but double-check:
            if (database == null) {
                Log.e(TAG, "ERROR: got null database object");
                return false;
            }

            // *Should* have been recovered:
            Cursor cursor = database.rawQuery("select * from sqlite_master;", null);

            if (cursor == null) {
                Log.e(TAG, "NOT EXPECTED: database.rawQuery() returned null cursor");
                return false;
            }

            // *Should* corrupt the database file that is already open:
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            try {
                // Attempt to write to corrupt database file *should* fail:
                database.execSQL("CREATE TABLE t1(a,b);");

                // NOT EXPECTED to get here:
                Log.e(TAG, "NOT EXPECTED: CREATE TABLE succeeded ");
                return false;
            } catch (SQLiteDatabaseCorruptException ex) {
                Log.v(TAG, "Caught SQLiteDatabaseCorruptException as expected OK");
            }

            // *Expected* to be closed now
            if (database.isOpen()) {
                Log.e(TAG, "NOT EXPECTED: database is still open");
                return false;
            }

            return true;
        } catch (Exception ex) {
            // Uncaught exception (not expected):
            Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
            return false;
        }
        finally {
            unencryptedDatabase.delete();
        }
    }

    @Override
    public String getName() {
        return "Corrupt Database Test";
    }
}
