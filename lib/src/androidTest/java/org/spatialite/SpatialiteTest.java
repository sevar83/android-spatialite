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


@RunWith(AndroidJUnit4.class)
public class SpatialiteTest {
    
    private static final int CURRENT_DATABASE_VERSION = 42;
    private SQLiteDatabase mDatabase;

    @Before
    public void setUp() {
        mDatabase = SQLiteDatabase.openOrCreateDatabase(":memory:", null);
        assertNotNull(mDatabase);
        mDatabase.setVersion(CURRENT_DATABASE_VERSION);
    }

    @After
    public void tearDown() {
        mDatabase.close();
    }

    private int getInt(String query) {
        try (Cursor c = mDatabase.rawQuery(query, new Object[]{})) {
            assertEquals(1, c.getCount());
            c.moveToFirst();
            return c.getInt(0);
        }
    }

    private String getString(String query) {
        try (Cursor c = mDatabase.rawQuery(query, new Object[]{})) {
            assertEquals(1, c.getCount());
            c.moveToFirst();
            return c.getString(0);
        }
    }

    @SmallTest
    @Test
    public void testVersion_Spatialite() {
        assertEquals("5.0.0-beta0", getString("SELECT spatialite_version()"));
    }

    @SmallTest
    @Test
    public void testVersion_Proj4() {
        assertEquals("Rel. 4.9.3, 15 August 2016", getString("SELECT proj4_version()"));
    }

    @SmallTest
    @Test
    public void testVersion_GEOS() {
        assertEquals("3.7.2-CAPI-1.11.2 b55d2125", getString("SELECT geos_version()"));
    }

    @SmallTest
    @Test
    public void testHasIconv() {
        assertEquals(1, getInt("SELECT HasIconv()"));
    }

    @SmallTest
    @Test
    public void testHasMathSQL() throws Exception {
        assertEquals(1, getInt("SELECT HasMathSQL()"));
    }

    /*@SmallTest
    @Test
    public void testHasGeoCallbacks() throws Exception {
        assertEquals(1, getInt("SELECT HasGeoCallbacks()"));
    }*/

    @SmallTest
    @Test
    public void testHasProj() throws Exception {
        assertEquals(1, getInt("SELECT HasProj()"));
    }

    @SmallTest
    @Test
    public void testHasGeos() throws Exception {
        assertEquals(1, getInt("SELECT HasGeos()"));
    }

    @SmallTest
    @Test
    public void testHasGeosAdvanced() throws Exception {
        assertEquals(1, getInt("SELECT HasGeosAdvanced()"));
    }

    /*@SmallTest
    @Test
    public void testHasGeosTrunk() throws Exception {
        assertEquals(1, getInt("SELECT HasGeosTrunk()"));
    }*/

    /*@SmallTest
    @Test
    public void testHasLwGeom() throws Exception {
        assertEquals(1, getInt("SELECT HasLwGeom()"));
    }*/

    @SmallTest
    @Test
    public void testHasLibXML2() throws Exception {
        assertEquals(1, getInt("SELECT HasLibXML2()"));
    }

    @SmallTest
    @Test
    public void testHasEpsg() throws Exception {
        assertEquals(1, getInt("SELECT HasEpsg()"));
    }

    @SmallTest
    @Test
    public void testHasFreeXL() throws Exception {
        assertEquals(1, getInt("SELECT HasFreeXL()"));
    }

    @SmallTest
    @Test
    public void testHasGeoPackage() throws Exception {
        assertEquals(1, getInt("SELECT HasGeoPackage()"));
    }

    @SmallTest
    @Test
    public void testHasRtTopo() {
        assertEquals(1, getInt("SELECT HasRtTopo()"));
    }

    @SmallTest
    @Test
    public void testHasGCP() {
        assertEquals(1, getInt("SELECT HasGCP()"));
    }

    @SmallTest
    @Test
    public void testHasTopology() {
        assertEquals(1, getInt("SELECT HasTopology()"));
    }

    @SmallTest
    @Test
    public void testHasKNN() {
        assertEquals(1, getInt("SELECT HasKNN()"));
    }

    @SmallTest
    @Test
    public void testHasRouting() {
        assertEquals(1, getInt("SELECT HasRouting()"));
    }

    // sql_stmt_tests/asbinary1.testcase
    @SmallTest
    @Test
    public void testBinary1() {
        assertEquals("0101000000000000000000F03F0000000000000040", getString("SELECT Hex(AsBinary(GeomFromText(\"Point(1 2)\", 4326)))"));
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
