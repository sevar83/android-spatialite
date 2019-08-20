/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// modified from original source see README at the top level of this project

package org.spatialite;

import android.database.Cursor;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.spatialite.database.SQLiteDatabase;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SmallTest;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

@SuppressWarnings("ResultOfMethodCallIgnored")
@RunWith(AndroidJUnit4.class)
public class SpatialiteTest {
    
    private static final int CURRENT_DATABASE_VERSION = 42;
    private SQLiteDatabase mDatabase;

    @Before
    public void setUp() throws Exception {
        mDatabase = SQLiteDatabase.openOrCreateDatabase(":memory:", null);
        assertNotNull(mDatabase);
        mDatabase.setVersion(CURRENT_DATABASE_VERSION);
    }

    @After
    public void tearDown() throws Exception {
        mDatabase.close();
    }

    @SmallTest
    @Test
    public void testVersion_Spatialite() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT spatialite_version()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("4.3.0a", c.getString(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testVersion_Proj4() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT proj4_version()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("Rel. 4.8.0, 6 March 2012", c.getString(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testVersion_GEOS() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT geos_version()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("3.4.2-CAPI-1.8.2 r0", c.getString(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasIconv() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasIconv()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasMathSQL() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasMathSQL()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    /*@SmallTest
    @Test
    public void testHasGeoCallbacks() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeoCallbacks()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }*/

    @SmallTest
    @Test
    public void testHasProj() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasProj()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasGeos() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeos()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasGeosAdvanced() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeosAdvanced()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasGeosTrunk() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeosTrunk()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    /*@SmallTest
    @Test
    public void testHasLwGeom() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasLwGeom()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }*/

    @SmallTest
    @Test
    public void testHasLibXML2() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasLibXML2()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasEpsg() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasEpsg()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasFreeXL() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasFreeXL()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    @Test
    public void testHasGeoPackage() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeoPackage()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    // sql_stmt_tests/asbinary1.testcase
    @SmallTest
    @Test
    public void testBinary1() {
        Cursor c = mDatabase.rawQuery("SELECT Hex(AsBinary(GeomFromText(\"Point(1 2)\", 4326)))", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("0101000000000000000000F03F0000000000000040", c.getString(0));
    }

    @SmallTest
    @Test
    public void testNestedFunctions() {
        mDatabase.execSQL("CREATE TABLE \"ln_data\" (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, geometry MULTILINESTRING)");
        Cursor c = mDatabase.rawQuery("select intersection(geometry, (select geometry from ln_data where id=62175)) as textintersection from ln_data", new Object[] {});
        assertEquals(0, c.getCount());
        c.moveToFirst();
    }
}
