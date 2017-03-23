package org.spatialite.sqlcipher;

import android.content.Context;
import android.support.annotation.CallSuper;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.spatialite.database.SQLiteDatabase;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import static android.content.ContentValues.TAG;

public abstract class BaseTest {

    public static String DATABASE_NAME = "test.db";

    protected Context targetContext;
    protected SQLiteDatabase database;
    protected File databasePath;

    @Before
    @CallSuper
    public void setUp() {
        targetContext = InstrumentationRegistry.getTargetContext();
        databasePath = targetContext.getDatabasePath(DATABASE_NAME);
        prepareDatabaseEnvironment();
        database = createDatabase(databasePath);
    }

    @After
    @CallSuper
    public void tearDown(){
        SQLiteDatabase.releaseMemory();
        database.close();
    }

    protected void prepareDatabaseEnvironment(){
        databasePath.getParentFile().mkdirs();

        if (databasePath.exists()){
            databasePath.delete();
        }
    }

    private SQLiteDatabase createDatabase(File databaseFile){
        return SQLiteDatabase.openOrCreateDatabase(databaseFile.getPath(), null);
    }

    protected void extractAssetToDatabaseDirectory(String fileName) throws IOException {

        int length;
        InputStream sourceDatabase = InstrumentationRegistry.getContext().getAssets().open(fileName);
        File destinationPath = databasePath;
        OutputStream destination = new FileOutputStream(destinationPath);

        byte[] buffer = new byte[4096];
        while((length = sourceDatabase.read(buffer)) > 0){
            destination.write(buffer, 0, length);
        }
        sourceDatabase.close();
        destination.flush();
        destination.close();
    }

    public void deleteDatabaseFileAndSiblings(String databaseName){

        File databaseFile = targetContext.getDatabasePath(databaseName);
        File databasesDirectory = new File(databaseFile.getParent());
        for(File file : databasesDirectory.listFiles()){
            file.delete();
        }
    }

    protected void log(String message){
        Log.i(TAG, message);
    }
}
