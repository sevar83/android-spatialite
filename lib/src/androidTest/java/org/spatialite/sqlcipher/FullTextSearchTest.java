package org.spatialite.sqlcipher;

import android.database.Cursor;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static junit.framework.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class FullTextSearchTest extends BaseTest {

    @Test
    public void fullTextSearch() {

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
        assertTrue(resultCount > 0);
    }
}
