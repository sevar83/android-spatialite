package org.spatialite.sqlcipher;

import org.junit.Test;
import org.junit.runner.RunWith;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import static junit.framework.Assert.assertEquals;

@RunWith(AndroidJUnit4.class)
public class EnableForeignKeySupportTest extends BaseTest {
    @Test
    public void enableForeignKeySupport() {
        String defaultValue = QueryHelper.singleValueFromQuery(database, "PRAGMA foreign_keys");
        database.execSQL("PRAGMA foreign_keys = ON;");
        String updatedValue = QueryHelper.singleValueFromQuery(database, "PRAGMA foreign_keys");
        assertEquals("0", defaultValue);
        assertEquals("1", updatedValue);
    }
}
