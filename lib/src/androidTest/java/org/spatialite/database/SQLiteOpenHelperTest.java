/*
 * Copyright (C) 2009 The Android Open Source Project
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

package org.spatialite.database;

import android.content.Context;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.spatialite.Cursor;

import static android.support.test.InstrumentationRegistry.getInstrumentation;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
/**
 * Test {@link SQLiteOpenHelper}.
 */
@RunWith(AndroidJUnit4.class)
@SmallTest
public class SQLiteOpenHelperTest {
    private static final String TEST_DATABASE_NAME = "database_test.db";
    private static final int TEST_VERSION = 1;
    private static final int TEST_ILLEGAL_VERSION = 0;
    private MockOpenHelper mOpenHelper;
    private SQLiteDatabase.CursorFactory mFactory = new SQLiteDatabase.CursorFactory() {
        public Cursor newCursor(SQLiteDatabase db, SQLiteCursorDriver masterQuery,
                                String editTable, SQLiteQuery query) {
            return new MockCursor(db, masterQuery, editTable, query);
        }
    };

    @Before
    public void setUp() throws Exception {
        SQLiteDatabase.loadLibs(getInstrumentation().getTargetContext());
        mOpenHelper = getOpenHelper();
    }

    @Test
    public void testConstructor() {
        Context targetContext = getInstrumentation().getTargetContext();
        new MockOpenHelper(targetContext, TEST_DATABASE_NAME, mFactory, TEST_VERSION);

        // Test with illegal version number.
        try {
            new MockOpenHelper(targetContext, TEST_DATABASE_NAME, mFactory, TEST_ILLEGAL_VERSION);
            fail("Constructor of SQLiteOpenHelp should throws a IllegalArgumentException here.");
        } catch (IllegalArgumentException e) {
        }

        // Test with null factory
        new MockOpenHelper(targetContext, TEST_DATABASE_NAME, null, TEST_VERSION);
    }

    @Test
    public void testGetDatabase() {
        SQLiteDatabase database = null;
        assertFalse(mOpenHelper.hasCalledOnOpen());
        // Test getReadableDatabase.
        database = mOpenHelper.getReadableDatabase((String) null);
        assertNotNull(database);
        assertTrue(database.isOpen());
        assertTrue(mOpenHelper.hasCalledOnOpen());

        // Database has been opened, so onOpen can not be invoked.
        mOpenHelper.resetStatus();
        assertFalse(mOpenHelper.hasCalledOnOpen());
        // Test getWritableDatabase.
        SQLiteDatabase database2 = mOpenHelper.getWritableDatabase((String) null);
        assertSame(database, database2);
        assertTrue(database.isOpen());
        assertFalse(mOpenHelper.hasCalledOnOpen());

        mOpenHelper.close();
        assertFalse(database.isOpen());

        // After close(), onOpen() will be invoked by getWritableDatabase.
        mOpenHelper.resetStatus();
        assertFalse(mOpenHelper.hasCalledOnOpen());
        SQLiteDatabase database3 = mOpenHelper.getWritableDatabase((String) null);
        assertNotNull(database);
        assertNotSame(database, database3);
        assertTrue(mOpenHelper.hasCalledOnOpen());
        assertTrue(database3.isOpen());
        mOpenHelper.close();
        assertFalse(database3.isOpen());
    }

    private MockOpenHelper getOpenHelper() {
        return new MockOpenHelper(getInstrumentation().getTargetContext(), TEST_DATABASE_NAME, mFactory, TEST_VERSION);
    }

    private class MockOpenHelper extends SQLiteOpenHelper {
        private boolean mHasCalledOnOpen = false;

        public MockOpenHelper(Context context, String name, SQLiteDatabase.CursorFactory factory, int version) {
            super(context, name, factory, version);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        }

        @Override
        public void onOpen(SQLiteDatabase db) {
            mHasCalledOnOpen = true;
        }

        public boolean hasCalledOnOpen() {
            return mHasCalledOnOpen;
        }

        public void resetStatus() {
            mHasCalledOnOpen = false;
        }
    }

    private class MockCursor extends SQLiteCursor {
        public MockCursor(SQLiteDatabase db, SQLiteCursorDriver driver, String editTable,
                SQLiteQuery query) {
            super(db, driver, editTable, query);
        }
    }
}
