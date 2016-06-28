package org.spatialite.testsuite.tests;

import android.database.Cursor;

import org.spatialite.database.SQLiteDatabase;

public class FullTextSearchTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {

        database.execSQL("CREATE VIRTUAL TABLE sites USING fts4(domain, url, title, meta_keys, body)");
        database.execSQL("CREATE TABLE keywords (keyword TEXT)");

        database.execSQL("insert into sites(domain, url, title, meta_keys, body) values(?, ?, ?, ?, ?)",
                         new Object[]{"sqlcipher.net", "http://sqlcipher.net",
                                      "Home - SQLCipher - Open Source Full Database Encryption for SQLite",
                                      "sqlcipher, sqlite", ""});
        database.execSQL("insert into keywords(keyword) values(?)", new Object[]{"SQLCipher"});
        database.execSQL("insert into keywords(keyword) values(?)", new Object[]{"SQLite"});

        String query = "SELECT keyword FROM keywords INNER JOIN sites ON sites.title MATCH keywords.keyword";
        Cursor result = database.rawQuery(query, new String[]{});
        int resultCount = 0;
        while (result.moveToNext()){
            String row = result.getString(0);
            if(row != null){
                resultCount++;
            }
        }
        result.close();
        return resultCount > 0;
    }

    @Override
    public String getName() {
        return "Full Text Search Test";
    }
}
