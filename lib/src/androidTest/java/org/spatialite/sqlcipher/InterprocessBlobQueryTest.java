package org.spatialite.sqlcipher;

import android.content.ContentResolver;
import android.net.Uri;

import org.junit.Test;

import static junit.framework.Assert.assertTrue;

public class InterprocessBlobQueryTest extends BaseTest {

    @Test
    public void customInterProcessBlob() {
        Uri providerUri = SpatialiteContentProvider2.CONTENT_URI;
        ContentResolver resolver = targetContext.getContentResolver();
        android.database.Cursor cursor = resolver.query(providerUri, null, null, null, null);
        StringBuilder buffer = new StringBuilder();
        while (cursor.moveToNext()) {
            buffer.append(cursor.getString(0));
            buffer.append(new String(cursor.getBlob(1)));
        }
        cursor.close();
        assertTrue(buffer.toString().length() > 0);
    }
}
