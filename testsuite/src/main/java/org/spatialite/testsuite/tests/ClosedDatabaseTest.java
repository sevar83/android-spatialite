package org.spatialite.testsuite.tests;

import android.util.Log;

import org.spatialite.DatabaseErrorHandler;
import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteException;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class ClosedDatabaseTest extends SQLCipherTest {

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

        File closedDatabasePath = SpatialiteApplication.getInstance().getDatabasePath("closed-db-test.db");

        boolean status = false;

        try {
            SQLiteDatabase database = SQLiteDatabase.openOrCreateDatabase(closedDatabasePath, "", null);

            database.close();

            status = execute_closed_database_tests(database);
        } catch (Exception e) {
            // Uncaught [unexpected] exception:
            Log.e(SpatialiteApplication.TAG, "Unexpected exception", e);
            return false;
        }
        finally {
            closedDatabasePath.delete();
        }

        if (!status) return false;

        status = false;

        final File corruptDatabase = SpatialiteApplication.getInstance().getDatabasePath("corrupt.db");

        // run closed database tests again in custom DatabaseErrorHandler:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // ugly trick from: http://stackoverflow.com/questions/5977735/setting-outer-variable-from-anonymous-inner-class
            final boolean[] inner_status_slot = new boolean[1];

            // on a database object that was NEVER actually opened (due to a corrupt database file):
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, SQLiteDatabase.CREATE_IF_NECESSARY, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        inner_status_slot[0] = execute_closed_database_tests(db);
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }

                    try {
                        corruptDatabase.delete();
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }
                }
            });

            status = inner_status_slot[0];

            // NOTE: database not expected to be null, but double-check:
            if (database == null) {
                Log.e(TAG, "ERROR: got null database object");
                return false;
            }

            database.close();
        } catch (Exception ex) {
            // Uncaught exception (not expected):
            Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
            return false;
        }
        finally {
            corruptDatabase.delete();
        }

        return status;
    }

    boolean execute_closed_database_tests(SQLiteDatabase database) {
        try {
            /* operations that check if db is closed (and throw IllegalStateException): */
            try {
                // should throw IllegalStateException:
                database.beginTransaction();

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.beginTransaction() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.beginTransaction() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.endTransaction();

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.endTransaction() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.endTransaction() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.setTransactionSuccessful();

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.setTransactionSuccessful() did NOT throw throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.setTransactionSuccessful() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.getVersion();

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.getVersion() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.getVersion() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.setVersion(111);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.setVersion() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.setVersion() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.getMaximumSize();

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.getMaximumSize() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.getMaximumSize() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.setMaximumSize(111);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.setMaximumSize() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.setMaximumSize() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.getPageSize();

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.getPageSize() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.getPageSize() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.setPageSize(111);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.setPageSize() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.setPageSize() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.compileStatement("SELECT 1;");

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.compileStatement() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.compileStatement() did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.query("t1", new String[]{"a", "b"}, null, null, null, null, null);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.query() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.query() did throw exception on closed database OK", e);
            }

            // TODO: cover more query functions

            try {
                // should throw IllegalStateException:
                database.execSQL("SELECT 1;");

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.execSQL(String) did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.execSQL(String) did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.execSQL("SELECT 1;", new Object[1]);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.execSQL(String, Object[]) did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.execSQL(String, Object[]) did throw exception on closed database OK", e);
            }

            try {
                // should throw IllegalStateException:
                database.rawExecSQL("SELECT 1;");

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.rawExecSQL() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.rawExecSQL() did throw exception on closed database OK", e);
            }

            /* operations that do not explicitly check if db is closed
             * ([should] throw SQLiteException on a closed database): */

//            try {
//                // should throw IllegalStateException:
//                database.setLocale(Locale.getDefault());
//
//                // should not get here:
//                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.setLocale() did NOT throw exception on closed database");
//                return false;
//            } catch (SQLiteException e) {
//                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.setLocale() did throw exception on closed database OK", e);
//            }

            try {
                // [should] throw an exception on a closed database:
                database.changePassword("new-password");

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.changePassword(String) did NOT throw exception on closed database");
                return false;
            } catch (SQLiteException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.changePassword(String) did throw exception on closed database OK", e);
            }

            try {
                // [should] throw an exception on a closed database:
                database.changePassword("new-password".toCharArray());

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.changePassword(char []) did NOT throw exception on closed database");
                return false;
            } catch (SQLiteException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.changePassword(char []) did throw exception on closed database OK", e);
            }

            try {
                // [should] throw an exception on a closed database:
                database.markTableSyncable("aa", "bb");

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.markTableSyncable(String, String) did NOT throw exception on closed database");
                return false;
            } catch (SQLiteException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.markTableSyncable(String, String) did throw exception on closed database OK", e);
            }

            try {
                // [should] throw an exception on a closed database:
                database.markTableSyncable("aa", "bb", "cc");

                // ...
                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.markTableSyncable(String, String, String) did NOT throw exception on closed database");
                return false;
            } catch (SQLiteException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.markTableSyncable(String, String, String) did throw exception on closed database OK", e);

                // SIGNAL that this test must be updated:
                //Log.e(SpatialiteApplication.TAG, "BEHAVIOR CHANGED - please update the test");
                //return false;
            }

            try {
                // should throw IllegalStateException [since it calls getVersion()]:
                database.needUpgrade(111);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.needUpgrade() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.needUpgrade() did throw exception on closed database OK", e);
            }

            /* operations that are NOT expected to throw an exception if the database is closed ([should] not crash) */


            /* XXX TODO: these functions should check the db state,
             * TBD either throw or simply return false if the db is closed */
            database.yieldIfContended();
            database.yieldIfContendedSafely();
            database.yieldIfContendedSafely(100);

            database.setLockingEnabled(false);
            database.setLockingEnabled(true);

            database.inTransaction();
            database.isDbLockedByCurrentThread();
            database.isDbLockedByOtherThreads();

            database.close();

            database.isReadOnly();
            database.isOpen();

            database.isInCompiledSqlCache("SELECT 1;");
            database.purgeFromCompiledSqlCache("SELECT 1;");
            database.resetCompiledSqlCache();

            database.getMaxSqlCacheSize();

            try {
                // should throw IllegalStateException:
                database.setMaxSqlCacheSize(111);

                // should not get here:
                Log.e(SpatialiteApplication.TAG, "SQLiteDatabase.setMaxSqlCacheSize() did NOT throw exception on closed database");
                return false;
            } catch (IllegalStateException e) {
                Log.v(SpatialiteApplication.TAG, "SQLiteDatabase.setMaxSqlCacheSize() did throw exception on closed database OK", e);
            }

        } catch (Exception e) {
            // Uncaught [unexpected] exception:
            Log.e(SpatialiteApplication.TAG, "Unexpected exception", e);
            return false;
        }

        return true;
    }

    @Override
    public String getName() {
        return "Closed Database Test";
    }
}
