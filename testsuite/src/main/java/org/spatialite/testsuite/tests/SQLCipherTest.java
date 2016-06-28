package org.spatialite.testsuite.tests;

import android.util.Log;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.SpatialiteApplication;

import java.io.File;

public abstract class SQLCipherTest {

    public abstract boolean execute(SQLiteDatabase database);
    public abstract String getName();
    public String TAG = getClass().getSimpleName();
    private TestResult result;

    private SQLiteDatabase database;

    protected void internalSetUp() {
        Log.i(TAG, "Before prepareDatabaseEnvironment");
        SpatialiteApplication.getInstance().prepareDatabaseEnvironment();
        Log.i(TAG, "Before getDatabasePath");
        File databasePath = SpatialiteApplication.getInstance().getDatabasePath(SpatialiteApplication.DATABASE_NAME);
        Log.i(TAG, "Before createDatabase");
        database = createDatabase(databasePath);
        Log.i(TAG, "Before setUp");
        setUp();
    }

    public TestResult run() {

        result = new TestResult(getName(), false);
        try {
            internalSetUp();
            result.setResult(execute(database));
            internalTearDown();
        } catch (Exception e) {
            Log.v(SpatialiteApplication.TAG, e.toString());
        }
        return result;
    }

    protected void setMessage(String message){
        result.setMessage(message);
    }

    private void internalTearDown(){
        SQLiteDatabase.releaseMemory();
        tearDown(database);
        database.close();
    }
    
    protected SQLiteDatabase createDatabase(File databasePath){
        Log.i(TAG, "Before SpatialiteApplication.getInstance().createDatabase");
        return SpatialiteApplication.getInstance().createDatabase(databasePath);
    }

    protected void setUp(){}

    protected void tearDown(SQLiteDatabase database){}

    protected void log(String message){
        Log.i(TAG, message);
    }
}
