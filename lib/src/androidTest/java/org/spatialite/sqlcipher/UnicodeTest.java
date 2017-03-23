package org.spatialite.sqlcipher;

import org.junit.Test;
import org.spatialite.database.SQLiteStatement;

import static junit.framework.Assert.assertEquals;

public class UnicodeTest extends BaseTest {
    @Test
    public void unicode() {
        //if (android.os.Build.VERSION.SDK_INT >= 23) { // Android M
            // This will crash on Android releases 1.X-5.X status due the following Android bug:
            // https://code.google.com/p/android/issues/detail?id=81341
            SQLiteStatement st = database.compileStatement("SELECT '\uD83D\uDE03'"); // SMILING FACE (MOUTH OPEN)
            String res = st.simpleQueryForString();
            String message = String.format("Returned value:%s", res);
            assertEquals("\uD83D\uDE03", res);
        //}
    }
}
