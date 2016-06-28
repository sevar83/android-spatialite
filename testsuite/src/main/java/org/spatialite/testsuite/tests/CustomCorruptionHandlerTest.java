package org.spatialite.testsuite.tests;

import android.util.Log;

import org.spatialite.DatabaseErrorHandler;
import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteDatabaseCorruptException;
import org.spatialite.database.SQLiteException;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public class CustomCorruptionHandlerTest extends SQLCipherTest {

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

        boolean status = false;

        final File corruptDatabase = SpatialiteApplication.getInstance().getDatabasePath("corrupt.db");

        // ugly trick from: http://stackoverflow.com/questions/5977735/setting-outer-variable-from-anonymous-inner-class
        final boolean[] inner_status_slot = new boolean[1];

        // normal recovery test with custom error handler that actually cleans it up:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // make sure custom onCorruption() function is called:
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, SQLiteDatabase.CREATE_IF_NECESSARY, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        Log.i(TAG, "Custom onCorruption() called");
                        inner_status_slot[0] = true;
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }

                    try {
                        // clean it up!
                        corruptDatabase.delete();
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                        inner_status_slot[0] = false;
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

        if (!status) return false;
        inner_status_slot[0] = status = false;

        // does not recover due to missing SQLiteDatabase.CREATE_IF_NECESSARY flag:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // make sure custom onCorruption() function is called:
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, 0, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        Log.i(TAG, "Custom onCorruption() called");
                        inner_status_slot[0] = true;
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }

                    try {
                        // clean it up!
                        corruptDatabase.delete();
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                        inner_status_slot[0] = false;
                    }
                }
            });

            // should not get here:
            Log.e(TAG, "UNEXPECTED RESULT: recovered from corrupt database without SQLiteDatabase.CREATE_IF_NECESSARY flag");
            database.close();
            corruptDatabase.delete();
            return false;
        } catch (SQLiteException ex) {
            Log.i(TAG, "Could not recover, as expected OK", ex);
            //Log.i(TAG, "inner_status_slot[0]: " + inner_status_slot[0])
            status = inner_status_slot[0];
        } catch (Exception ex) {
            // Uncaught exception (not expected):
            Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
            return false;
        }

        if (!status) return false;
        inner_status_slot[0] = status = false;

        // attempt to recover with custom error handler but database is not cleaned up:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // make sure custom onCorruption() function is called:
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, SQLiteDatabase.CREATE_IF_NECESSARY, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        inner_status_slot[0] = true;
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }
                }
            });

            // should not get here:
            Log.e(TAG, "UNEXPECTED RESULT: recovered from corrupt database that was not cleaned up");
            database.close();
            corruptDatabase.delete();
            return false;
        } catch (SQLiteException ex) {
            Log.i(TAG, "Could not recover, as expected OK", ex);
            status = inner_status_slot[0];
        } catch (Exception ex) {
            // Uncaught exception (not expected):
            Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
            return false;
        }

        if (!status) return false;
        inner_status_slot[0] = status = false;

        // custom error handler throws:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // make sure custom onCorruption() function is called:
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, SQLiteDatabase.CREATE_IF_NECESSARY, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        inner_status_slot[0] = true;
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }

                    throw new RuntimeException("abort");
                }
            });

            // should not get here:
            Log.e(TAG, "UNEXPECTED RESULT: recovered from corrupt database that was not cleaned up");
            database.close();
            corruptDatabase.delete();
            return false;
        } catch (RuntimeException ex) {
            Log.v(TAG, "Caught RuntimeException as thrown by custom error handler OK");
            status = inner_status_slot[0];
        } catch (Exception ex) {
            // Uncaught exception (not expected):
            Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
            return false;
        }

        if (!status) return false;
        inner_status_slot[0] = status = false;

        // extra fun:
        final boolean[] inner_fun_slot = new boolean[1];

        // tell custom error handler NOT to delete corrupt database during sql query:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // make sure custom onCorruption() function is called:
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, SQLiteDatabase.CREATE_IF_NECESSARY, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        Log.i(TAG, "Custom onCorruption() called");
                        inner_status_slot[0] = true;
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }

                    if (inner_fun_slot[0]) return;

                    try {
                        // clean it up!
                        corruptDatabase.delete();
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                        inner_status_slot[0] = false;
                    }
                }
            });

            // NOTE: database not expected to be null, but double-check:
            if (database == null) {
                Log.e(TAG, "ERROR: got null database object");
                return false;
            }

            if (!inner_status_slot[0]) {
                Log.e(TAG, "ERROR: Custom onCorruption() NOT called");
                database.close();
                return false;
            }

            // tell custom error handler NOT to delete database:
            inner_fun_slot[0] = true;

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

            // NOTE: the database is NOT closed, try it again
            if (!database.isOpen()) {
                Log.e(TAG, "BEHAVIOR CHANGED: database was closed after sql encountered corruption but error handler did not delete the database");
                return false;
            }

            // custom error handler back to normal:
            inner_fun_slot[0] = false;

            try {
                // Attempt to write to corrupt database file *should* fail:
                database.execSQL("CREATE TABLE t1(a,b);");

                // NOT EXPECTED to get here:
                Log.e(TAG, "NOT EXPECTED: CREATE TABLE succeeded ");
                return false;
            } catch (SQLiteDatabaseCorruptException ex) {
                Log.v(TAG, "Caught SQLiteDatabaseCorruptException as expected OK");
            }

            // For some reason, database is still open here.
            if (!database.isOpen()) {
                Log.e(TAG, "BEHAVIOR CHANGED: database closed here");
                return false;
            }

            database.close();

            status = true;

        } catch (Exception ex) {
            // Uncaught exception (not expected):
            Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
            return false;
        }
        finally {
            corruptDatabase.delete();
        }

        if (!status) return false;
        inner_status_slot[0] = status = false;
        inner_fun_slot[0] = false;

        // tell custom error handler to throw runtime error during sql query:
        try {
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            // make sure custom onCorruption() function is called:
            SQLiteDatabase database = SQLiteDatabase.openDatabase(corruptDatabase.getPath(), "", null, SQLiteDatabase.CREATE_IF_NECESSARY, null, new DatabaseErrorHandler() {
                @Override
                public void onCorruption(SQLiteDatabase db) {
                    try {
                        Log.i(TAG, "Custom onCorruption() called");
                        inner_status_slot[0] = true;
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                    }

                    if (inner_fun_slot[0]) throw new RuntimeException("abort");

                    try {
                        // clean it up!
                        corruptDatabase.delete();
                    } catch (Exception ex) {
                        // Uncaught exception (not expected):
                        Log.e(TAG, "UNEXPECTED EXCEPTION", ex);
                        inner_status_slot[0] = false;
                    }
                }
            });

            // NOTE: database not expected to be null, but double-check:
            if (database == null) {
                Log.e(TAG, "ERROR: got null database object");
                return false;
            }

            if (!inner_status_slot[0]) {
                Log.e(TAG, "ERROR: Custom onCorruption() NOT called");
                database.close();
                return false;
            }

            // tell custom error handler NOT to delete database:
            inner_fun_slot[0] = true;

            // *Should* corrupt the database file that is already open:
            SpatialiteApplication.getInstance().extractAssetToDatabaseDirectory("corrupt.db");

            try {
                // Attempt to write to corrupt database file *should* fail:
                database.execSQL("CREATE TABLE t1(a,b);");

                // NOT EXPECTED to get here:
                Log.e(TAG, "NOT EXPECTED: CREATE TABLE succeeded ");
                return false;
            } catch (RuntimeException ex) {
                Log.v(TAG, "Caught RuntimeException as thrown by custom error handler OK");
            }

            // NOTE: the database is NOT expected to closed
            if (!database.isOpen()) {
                Log.e(TAG, "BEHAVIOR CHANGED: database was closed after sql encountered corruption but error handler did not delete the database");
                return false;
            }

            database.close();

            status = true;

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

    @Override
    public String getName() {
        return "Custom Corruption Handler Test";
    }
}
