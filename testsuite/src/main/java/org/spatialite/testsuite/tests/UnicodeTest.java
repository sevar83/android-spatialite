package org.spatialite.testsuite.tests;

import android.util.Log;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.database.SQLiteStatement;

public class UnicodeTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {
        //if (android.os.Build.VERSION.SDK_INT >= 23) { // Android M
            // This will crash on Android releases 1.X-5.X status due the following Android bug:
            // https://code.google.com/p/android/issues/detail?id=81341
            SQLiteStatement st = database.compileStatement("SELECT '\uD83D\uDE03'"); // SMILING FACE (MOUTH OPEN)
            String res = st.simpleQueryForString();
            String message = String.format("Returned value:%s", res);
            setMessage(message);
            Log.i(TAG, message);
            return res.equals("\uD83D\uDE03");
        //}
    }

    @Override
    public String getName() {
        return "Unicode Test";
    }
}
