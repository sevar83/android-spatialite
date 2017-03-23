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
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import org.spatialite.database.SQLiteDatabase;

@SuppressWarnings("ResultOfMethodCallIgnored")
public class SpatialiteTest extends AndroidTestCase {
    
    private static final int CURRENT_DATABASE_VERSION = 42;
    private SQLiteDatabase mDatabase;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDatabase = SQLiteDatabase.openOrCreateDatabase(":memory:", null);
        assertNotNull(mDatabase);
        mDatabase.setVersion(CURRENT_DATABASE_VERSION);
    }

    @Override
    protected void tearDown() throws Exception {
        mDatabase.close();
        super.tearDown();
    }

    @SmallTest
    public void testVersion_Spatialite() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT spatialite_version()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("4.3.0a", c.getString(0));
        c.close();
    }

    @SmallTest
    public void testVersion_Proj4() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT proj4_version()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("Rel. 4.8.0, 6 March 2012", c.getString(0));
        c.close();
    }

    @SmallTest
    public void testVersion_GEOS() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT geos_version()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("3.4.2-CAPI-1.8.2 r0", c.getString(0));
        c.close();
    }

    @SmallTest
    public void testHasIconv() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasIconv()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasMathSQL() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasMathSQL()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    /*@SmallTest
    public void testHasGeoCallbacks() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeoCallbacks()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }*/

    @SmallTest
    public void testHasProj() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasProj()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasGeos() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeos()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasGeosAdvanced() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeosAdvanced()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasGeosTrunk() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeosTrunk()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    /*@SmallTest
    public void testHasLwGeom() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasLwGeom()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }*/

    @SmallTest
    public void testHasLibXML2() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasLibXML2()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasEpsg() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasEpsg()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasFreeXL() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasFreeXL()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    @SmallTest
    public void testHasGeoPackage() throws Exception {
        Cursor c = mDatabase.rawQuery("SELECT HasGeoPackage()", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals(1, c.getInt(0));
        c.close();
    }

    // sql_stmt_tests/asbinary1.testcase
    @SmallTest
    public void testBinary1() {
        Cursor c = mDatabase.rawQuery("SELECT Hex(AsBinary(GeomFromText(\"Point(1 2)\", 4326)))", new Object[] {});
        assertEquals(1, c.getCount());
        c.moveToFirst();
        assertEquals("0101000000000000000000F03F0000000000000040", c.getString(0));
    }
}
