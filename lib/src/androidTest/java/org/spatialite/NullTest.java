package org.spatialite;

import android.content.Context;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.spatialite.database.SQLiteDatabase;

import java.io.File;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import static org.junit.Assert.assertNotNull;

@RunWith(AndroidJUnit4.class)
public class NullTest {

    private SQLiteDatabase mDatabase;
    private File mDatabaseFile;

    @Before
    public void setUp() throws Exception {
        File dbDir = ApplicationProvider.getApplicationContext().getDir(this.getClass().getName(), Context.MODE_PRIVATE);
        mDatabaseFile = new File(dbDir, "database_test.db");
        if (mDatabaseFile.exists()) {
            mDatabaseFile.delete();
        }
        mDatabase = SQLiteDatabase.create(null);
        assertNotNull(mDatabase);
    }

    @After
    public void tearDown() throws Exception {
        mDatabase.close();
    }

    @SmallTest
    @Test
    public void testNullsInsertion() {
        mDatabase.execSQL(
                "CREATE TABLE `TEST` (" +
                        "`ID`       INTEGER PRIMARY KEY AUTOINCREMENT," +
                        "`S1`      VARCHAR(50)," +
                        "`S2`      VARCHAR(50)," +
                        "`S3`      VARCHAR(50)" +
                        ");" +

                "insert into TEST(S1, S2, S3) values('Ein Text', '', null);"
        );
    }
}
