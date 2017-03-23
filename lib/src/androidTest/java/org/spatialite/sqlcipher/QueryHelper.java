package org.spatialite.sqlcipher;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;


public class QueryHelper {

    public static String singleValueFromQuery(SQLiteDatabase database, String query){
        Cursor cursor = database.rawQuery(query, new String[]{});
        String value = "";
        if(cursor != null){
            cursor.moveToFirst();
            value = cursor.getString(0);
            cursor.close();
        }
        return value;
    }

    public static int singleIntegerValueFromQuery(SQLiteDatabase database, String query){
        Cursor cursor = database.rawQuery(query, new String[]{});
        int value = 0;
        if(cursor != null){
            cursor.moveToFirst();
            value = cursor.getInt(0);
            cursor.close();
        }
        return value;
    }
}
