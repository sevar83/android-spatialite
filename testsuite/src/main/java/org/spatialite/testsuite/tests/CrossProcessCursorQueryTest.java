package org.spatialite.testsuite.tests;

import android.app.Activity;
import android.net.Uri;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.SpatialiteApplication;
import org.spatialite.testsuite.SpatialiteContentProvider;

public class CrossProcessCursorQueryTest extends SQLCipherTest {
    
    @Override
    public boolean execute(SQLiteDatabase database) {

        Activity activity = SpatialiteApplication.getInstance().getCurrentActivity();
        Uri providerUri = SpatialiteContentProvider.CONTENT_URI;
        android.database.Cursor cursor = activity.managedQuery(providerUri, null, null, null, null);
        StringBuilder buffer = new StringBuilder();
        while (cursor.moveToNext()) {
            buffer.append(cursor.getString(0));
            buffer.append(cursor.getString(1));
        }
        cursor.close();
        return buffer.toString().length() > 0;
    }



    @Override
    public String getName() {
        return "Custom Cross Process Cursor Test";
    }
}
