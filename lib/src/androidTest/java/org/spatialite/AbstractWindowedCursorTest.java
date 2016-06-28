/*
 * Copyright (C) 2008 The Android Open Source Project
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

package org.spatialite;

import android.database.CharArrayBuffer;
import android.database.CursorIndexOutOfBoundsException;
import android.database.StaleDataException;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.spatialite.database.SQLiteDatabase;

import java.util.Arrays;

import static android.support.test.InstrumentationRegistry.getInstrumentation;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class AbstractWindowedCursorTest {
    private static final String TEST_STRING = "TESTSTRING";
    private static final int COLUMN_INDEX0 = 0;
    private static final int COLUMN_INDEX1 = 1;
    private static final int ROW_INDEX0 = 0;
    private static final int TEST_COLUMN_COUNT = 7;
    private MockAbstractWindowedCursor mCursor;
    private CursorWindow mWindow;

    @Before
    public void setUp() throws Exception {
        SQLiteDatabase.loadLibs(getInstrumentation().getTargetContext());

        mCursor = new MockAbstractWindowedCursor();
        mWindow = new CursorWindow(false);
    }

    @After
    public void tearDown() throws Exception {
        mCursor.close();
        mWindow.close();
    }

    @Test
    public void testIsNull() {
        mCursor.setWindow(mWindow);

        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        mCursor.moveToFirst();
        assertTrue(mCursor.isNull(COLUMN_INDEX0));
        assertTrue(mWindow.allocRow());

        String str = "abcdefg";
        assertTrue(mWindow.putString(str, ROW_INDEX0, COLUMN_INDEX0));
        assertFalse(mCursor.isNull(COLUMN_INDEX0));
    }

    @Test
    public void testIsBlob() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        mCursor.moveToFirst();
        assertFalse(mCursor.isBlob(COLUMN_INDEX0));

        String str = "abcdefg";
        assertTrue(mWindow.putString(str, ROW_INDEX0, COLUMN_INDEX0));
        assertTrue(mWindow.putBlob(new byte[10], ROW_INDEX0, COLUMN_INDEX1));
        assertTrue(mCursor.isBlob(COLUMN_INDEX1));
    }

    @Test
    public void testHasWindow() {
        assertFalse(mCursor.hasWindow());
        assertNull(mCursor.getWindow());

        mCursor.setWindow(mWindow);
        assertTrue(mCursor.hasWindow());
        assertSame(mWindow, mCursor.getWindow());

        mCursor.setWindow(null);
        assertFalse(mCursor.hasWindow());
        assertNull(mCursor.getWindow());
    }

    @Test
    public void testGetString() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        mCursor.moveToFirst();
        String str = "abcdefg";
        assertTrue(mWindow.putString(str, ROW_INDEX0, COLUMN_INDEX0));
        assertEquals(str, mCursor.getString(COLUMN_INDEX0));
    }

    @Test
    public void testGetShort() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        mCursor.moveToFirst();
        short shortNumber = 10;
        assertTrue(mWindow.putLong((long) shortNumber, ROW_INDEX0, COLUMN_INDEX0));
        assertEquals(shortNumber, mCursor.getShort(COLUMN_INDEX0));
    }

    @Test
    public void testGetLong() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        mCursor.moveToFirst();
        long longNumber = 10;
        assertTrue(mWindow.putLong(longNumber, ROW_INDEX0, COLUMN_INDEX0));
        assertEquals(longNumber, mCursor.getLong(COLUMN_INDEX0));
    }

    @Test
    public void testGetInt() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        mCursor.moveToFirst();
        int intNumber = 10;
        assertTrue(mWindow.putLong((long) intNumber, ROW_INDEX0, COLUMN_INDEX0));
        assertEquals(intNumber, mCursor.getInt(COLUMN_INDEX0));
    }

    @Test
    public void testGetFloat() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        mCursor.moveToFirst();
        float f1oatNumber = 1.26f;
        assertTrue(mWindow.putDouble((double) f1oatNumber, ROW_INDEX0, COLUMN_INDEX0));
        assertEquals(f1oatNumber, mCursor.getFloat(COLUMN_INDEX0), (float) TestConstants.EPSILON);
    }

    @Test
    public void testGetDouble() {
        mCursor.setWindow(mWindow);
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());

        double db1 = 1.26;
        assertTrue(mWindow.putDouble(db1, ROW_INDEX0, COLUMN_INDEX0));

        double db2 = mWindow.getDouble(ROW_INDEX0, COLUMN_INDEX0);
        assertEquals(db1, db2, TestConstants.EPSILON);

        mCursor.moveToFirst();
        double cd = mCursor.getDouble(COLUMN_INDEX0);
        assertEquals(db1, cd, TestConstants.EPSILON);
    }

    @Test
    public void testGetBlob() {
        byte TEST_VALUE = 3;
        byte BLOB_SIZE = 100;
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());
        assertTrue(mWindow.putString("", ROW_INDEX0, COLUMN_INDEX0));

        byte[] blob = new byte[BLOB_SIZE];
        Arrays.fill(blob, TEST_VALUE);
        assertTrue(mWindow.putBlob(blob, ROW_INDEX0, COLUMN_INDEX1));

        mCursor.setWindow(mWindow);
        mCursor.moveToFirst();

        byte[] targetBuffer = mCursor.getBlob(COLUMN_INDEX1);
        assertEquals(BLOB_SIZE, targetBuffer.length);
        assertTrue(Arrays.equals(blob, targetBuffer));
    }

    @Ignore("Native crash in copyStringToBuffer")
    @Test
    public void testCopyStringToBuffer() {
        assertTrue(mWindow.setNumColumns(TEST_COLUMN_COUNT));
        assertTrue(mWindow.allocRow());
        assertTrue(mWindow.putString(TEST_STRING, ROW_INDEX0, COLUMN_INDEX0));
        assertTrue(mWindow.putString("", ROW_INDEX0, COLUMN_INDEX1));

        mCursor.setWindow(mWindow);
        mCursor.moveToFirst();

        CharArrayBuffer charArrayBuffer = new CharArrayBuffer(TEST_STRING.length());

        mCursor.copyStringToBuffer(COLUMN_INDEX0, charArrayBuffer);
        assertEquals(TEST_STRING.length(), charArrayBuffer.sizeCopied);
        assertTrue(Arrays.equals(TEST_STRING.toCharArray(), charArrayBuffer.data));

        Arrays.fill(charArrayBuffer.data, '\0');
        mCursor.copyStringToBuffer(COLUMN_INDEX1, charArrayBuffer);
        assertEquals(0, charArrayBuffer.sizeCopied);
    }

    @Test
    public void testCheckPosition() {
        try {
            mCursor.checkPosition();
            fail("testCheckPosition failed");
        } catch (CursorIndexOutOfBoundsException e) {
            // expected
        }

        try {
            assertTrue(mCursor.moveToPosition(mCursor.getCount() - 1));
            mCursor.checkPosition();
            fail("testCheckPosition failed");
        } catch (StaleDataException e) {
            // expected
        }

        assertTrue(mCursor.moveToPosition(mCursor.getCount() - 1));
        mCursor.setWindow(mWindow);
        mCursor.checkPosition();
    }

    private class MockAbstractWindowedCursor extends AbstractWindowedCursor {

        public MockAbstractWindowedCursor() {
        }

        @Override
        public String[] getColumnNames() {
            return new String[] {
                    "col1", "col2", "col3"
            };
        }

        @Override
        public int getCount() {
            return 1;
        }

        @Override
        protected void checkPosition() {
            super.checkPosition();
        }
    }
}
