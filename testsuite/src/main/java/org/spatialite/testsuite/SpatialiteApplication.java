package org.spatialite.testsuite;

import android.app.Activity;
import android.app.Application;
import android.util.Log;

import org.spatialite.database.SQLiteDatabase;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class SpatialiteApplication extends Application {

    public static String DATABASE_NAME = "test.db";
    public static String DATABASE_PASSWORD = "test";
    private static SpatialiteApplication instance;
    private Activity activity;
    public static final String TAG = "Zetetic";
    public static final String ONE_X_DATABASE = "1x.db";
    public static final String ONE_X_USER_VERSION_DATABASE = "1x-user-version.db";
    public static final String UNENCRYPTED_DATABASE = "unencrypted.db";

    public SpatialiteApplication(){
        instance = this;
    }

    public static SpatialiteApplication getInstance(){
        return instance;
    }

    public void setCurrentActivity(Activity activity){
        this.activity = activity;
    }

    public Activity getCurrentActivity(){
        return activity;
    }

    public void prepareDatabaseEnvironment(){
        Log.i(TAG, "Entered prepareDatabaseEnvironment");
        Log.i(TAG, "Before getDatabasePath");
        File databaseFile = getDatabasePath(DATABASE_NAME);
        Log.i(TAG, "Before mkdirs on parent of database path");
        databaseFile.getParentFile().mkdirs();

        if(databaseFile.exists()){
            Log.i(TAG, "Before delete of database file");
            databaseFile.delete();
        }
//        databaseFile.mkdirs();
//        databaseFile.delete();
    }

    public SQLiteDatabase createDatabase(File databaseFile){
        Log.i(TAG, "Entered SpatialiteApplication::createDatabase");
        Log.i(TAG, "Before SQLiteDatabase.openOrCreateDatabase");
        return SQLiteDatabase.openOrCreateDatabase(databaseFile.getPath(), null);
    }

    public void extractAssetToDatabaseDirectory(String fileName) throws IOException {

        int length;
        InputStream sourceDatabase = SpatialiteApplication.getInstance().getAssets().open(fileName);
        File destinationPath = SpatialiteApplication.getInstance().getDatabasePath(fileName);
        OutputStream destination = new FileOutputStream(destinationPath);

        byte[] buffer = new byte[4096];
        while((length = sourceDatabase.read(buffer)) > 0){
            destination.write(buffer, 0, length);
        }
        sourceDatabase.close();
        destination.flush();
        destination.close();
    }

    public void delete1xDatabase() {
        
        File databaseFile = getInstance().getDatabasePath(ONE_X_DATABASE);
        databaseFile.delete();
    }

    public void deleteDatabaseFileAndSiblings(String databaseName){

        File databaseFile = SpatialiteApplication.getInstance().getDatabasePath(databaseName);
        File databasesDirectory = new File(databaseFile.getParent());
        for(File file : databasesDirectory.listFiles()){
            file.delete();
        }
    }
}
