package org.spatialite.testsuite.tests;

import org.spatialite.Cursor;
import org.spatialite.database.SQLiteDatabase;

public class ICUTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {
        String expected = "КАКОЙ-ТО КИРИЛЛИЧЕСКИЙ ТЕКСТ"; // SOME Cyrillic TEXT
        String actual = "";

        Cursor result = database.rawQuery("select UPPER('Какой-то кириллический текст') as u1", new String[]{});
        if (result != null) {
            result.moveToFirst();
            actual = result.getString(0);
            result.close();
        }
        return actual.equals(expected);
    }

    @Override
    public String getName() {
        return "ICU Test";
    }
}
