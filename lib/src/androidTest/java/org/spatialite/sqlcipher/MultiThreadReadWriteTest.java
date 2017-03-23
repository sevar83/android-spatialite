package org.spatialite.sqlcipher;

import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.util.Log;

import org.junit.Test;
import org.spatialite.database.SQLiteDatabase;

import java.util.ArrayList;
import java.util.List;

import static junit.framework.Assert.assertTrue;

public class MultiThreadReadWriteTest extends BaseTest {

    private static final String TAG = "MultiThreadTest";

    private static SQLiteDatabase instance;
    ThreadExecutor executor;
    int recordsPerThread = 20;
    int threadCount = 40;

    @Test
    public void multiThreadedReadWrite() {
        database.close();
        executor = new ThreadExecutor(threadCount, recordsPerThread, DatabaseAccessType.Singleton);
        assertTrue(executor.run());
    }

    synchronized SQLiteDatabase getDatabase(DatabaseAccessType accessType) {
        if (accessType == DatabaseAccessType.InstancePerRequest) {
            return SQLiteDatabase.openOrCreateDatabase(databasePath, null);
        } else if (accessType == DatabaseAccessType.Singleton) {
            if (instance == null) {
                instance = SQLiteDatabase.openOrCreateDatabase(databasePath, null);
            }
            return instance;
        }
        return null;
    }

    synchronized void closeDatabase(SQLiteDatabase database, DatabaseAccessType accessType){
        if(accessType == DatabaseAccessType.InstancePerRequest){
            database.close();
        }
    }

    private class Writer implements Runnable {

        int id;
        int size;
        private DatabaseAccessType accessType;

        public Writer(int id, int size, DatabaseAccessType accessType) {
            this.id = id;
            this.size = size;
            this.accessType = accessType;
        }

        @Override
        public void run() {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
            Log.i(TAG, String.format("writer thread %d beginning", id));
            // not expected to throw:
            SQLiteDatabase writer = getDatabase(accessType);

            try {
                writer.execSQL("create table if not exists t1(a,b)");

                for (int index = 0; index < size; index++) {
                    Log.i(TAG, String.format("writer thread %d - insert data for row:%d", id, index));
                    writer.execSQL("insert into t1(a,b) values(?, ?)",
                        new Object[]{"one for the money", "two for the show"});
                }
            } catch (SQLiteException ex) { Log.e(TAG, "caught exception, bailing", ex); }
              catch (IllegalStateException ex) { Log.e(TAG, "caught exception, bailing", ex); }

            closeDatabase(writer, accessType);
            Log.i(TAG, String.format("writer thread %d terminating", id));
        }
    }

    private class Reader implements Runnable {

        int id;
        private DatabaseAccessType accessType;

        public Reader(int id, DatabaseAccessType accessType) {
            this.id = id;
            this.accessType = accessType;
        }

        @Override
        public void run() {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_BACKGROUND);
            Log.i(TAG, String.format("reader thread %d beginning", id));
            SQLiteDatabase reader = getDatabase(accessType);
            int currentCount = 0;
            int updatedCount = getCurrentTableCount(reader);
            while (currentCount == 0 || currentCount != updatedCount) {
                logRecordsBetween(reader, currentCount, updatedCount);
                currentCount = updatedCount;
                updatedCount = getCurrentTableCount(reader);
            }
            closeDatabase(reader, accessType);
            Log.i(TAG, String.format("reader thread %d terminating", id));
        }

        synchronized void logRecordsBetween(SQLiteDatabase reader, int start, int end) {
            if(!reader.isOpen()) return;
            Cursor results = null;
            try {
                results = reader.rawQuery("select rowid, * from t1 where rowid between ? and ?",
                    new String[]{String.valueOf(start), String.valueOf(end)});
            } catch (IllegalStateException ex) { Log.e(TAG, "caught exception, bailing", ex); }
              catch (SQLiteException ex) { Log.e(TAG, "caught exception, bailing", ex); }

            if (results != null) {
                Log.i(TAG, String.format("reader thread %d - writing results %d to %d", id, start, end));
                while (results.moveToNext()) {
                    Log.i(TAG, String.format("reader thread %d - record:%d, a:%s b:%s",
                            id, results.getInt(0), results.getString(1), results.getString(2)));
                }
                results.close();
            }
        }

        synchronized int getCurrentTableCount(SQLiteDatabase database) {
            int count = 0;

            if (!database.isOpen()) return -1;
            Cursor cursor = null;

            // I. Attempt database.rawQuery()-bail (explicitly return 0) if it throws:
            try {
                cursor = database.rawQuery("select count(*) from t1;", new String[]{});
            } catch (IllegalStateException ex) { Log.e(TAG, "caught exception, bailing with count = " + count, ex); return 0; }
              catch (SQLiteException ex) { Log.e(TAG, "caught exception, bailing", ex); return 0; }

            // II. Attempt to get the count from the cursor:
            if (cursor != null) {
                try {
                    if (cursor.moveToFirst()) {
                        count = cursor.getInt(0);
                    }
                    cursor.close();
                } catch (IllegalStateException ex) { Log.e(TAG, "caught exception, bailing with count = " + count, ex); }
            }

            return count;
        }
    }

    private class ThreadExecutor {

        private int threadCount;
        private int recordsPerThread;
        private DatabaseAccessType accessType;
        List<Thread> readerThreads;
        List<Thread> writerThreads;
        boolean status = false;

        public ThreadExecutor(int threads, int recordsPerThread, DatabaseAccessType accessType) {

            if (threads % 2 != 0) {
                throw new IllegalArgumentException("Threads must be split evenly between readers and writers");
            }
            this.threadCount = threads / 2;
            this.recordsPerThread = recordsPerThread;
            this.accessType = accessType;
            readerThreads = new ArrayList<Thread>();
            writerThreads = new ArrayList<Thread>();
        }

        public boolean run() {

            for (int index = 0; index < threadCount; index++) {
                readerThreads.add(new Thread(new Reader(index, accessType)));
                writerThreads.add(new Thread(new Writer(index, recordsPerThread, accessType)));
            }
            for (int index = 0; index < threadCount; index++) {
                Log.i(TAG, String.format("Starting writer thread %d", index));
                writerThreads.get(index).start();
            }
            try {
                for (int index = 0; index < threadCount; index++) {
                    Log.i(TAG, String.format("Starting reader thread %d", index));
                    readerThreads.get(index).start();
                }
                Log.i(TAG, String.format("Request reader thread %d to join", 0));
                readerThreads.get(threadCount - 1).join();
                status = true;
            } catch (InterruptedException e) {
                Log.i(TAG, "Thread join failure", e);
            }
            if(accessType == DatabaseAccessType.Singleton){
                getDatabase(accessType).close();
            }
            return status;
        }
    }

    private enum DatabaseAccessType {
        Singleton,
        InstancePerRequest
    }
}
