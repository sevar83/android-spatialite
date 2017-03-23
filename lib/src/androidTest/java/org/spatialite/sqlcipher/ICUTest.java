package org.spatialite.sqlcipher;

import android.database.Cursor;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;

import static junit.framework.Assert.assertEquals;

@Ignore("ICU not supported. Native ICU library is not included since 2.0")
@RunWith(AndroidJUnit4.class)
public class ICUTest extends BaseTest {
    @Test
    public void icu() {
        String expected = "КАКОЙ-ТО КИРИЛЛИЧЕСКИЙ ТЕКСТ"; // SOME Cyrillic TEXT
        String actual = "";

        Cursor result = database.rawQuery("select UPPER('Какой-то кириллический текст') as u1", new String[]{});
        if (result != null) {
            result.moveToFirst();
            actual = result.getString(0);
            result.close();
        }
        assertEquals(expected, actual);
    }
}
