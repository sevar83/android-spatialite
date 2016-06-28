package org.spatialite.testsuite.tests;

import android.os.AsyncTask;
import android.os.Build;
import android.util.Log;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.SpatialiteApplication;

import java.util.ArrayList;
import java.util.List;

public class TestSuiteRunner extends AsyncTask<ResultNotifier, TestResult, Void> {

    private ResultNotifier notifier;

    @Override
    protected Void doInBackground(ResultNotifier... resultNotifiers) {
        this.notifier = resultNotifiers[0];
        Log.i(SpatialiteApplication.TAG, String.format("Running test suite on %s platform", Build.CPU_ABI));
        runSuite();
        return null;
    }

    @Override
    protected void onProgressUpdate(TestResult... values) {
        notifier.send(values[0]);
    }

    @Override
    protected void onPostExecute(Void aVoid) {
        notifier.complete();
    }

    private void runSuite(){

        SQLiteDatabase.loadLibs(SpatialiteApplication.getInstance());
        for(SQLCipherTest test : getTestsToRun()){
            try{
                Log.i(SpatialiteApplication.TAG, "Running test:" + test.getName());
                publishProgress(test.run());
            }catch (Throwable e){
                Log.i(SpatialiteApplication.TAG, e.toString());
                publishProgress(new TestResult(test.getName(), false, e.toString()));
            }
        }
    }

    private List<SQLCipherTest> getTestsToRun(){

        // SV: Disabled all encryption-related tests

        List<SQLCipherTest> tests = new ArrayList<SQLCipherTest>();
        //tests.add(new PragmaCipherVersionTest());
        tests.add(new UnicodeTest());
        //tests.add(new FIPSTest());
        tests.add(new QueryIntegerToStringTest());
        tests.add(new QueryFloatToStringTest());
        tests.add(new ClosedDatabaseTest());
        tests.add(new AttachDatabaseTest());
        //tests.add(new CipherMigrateTest());
        tests.add(new GetTypeFromCrossProcessCursorWrapperTest());
        //tests.add(new InvalidPasswordTest());
        tests.add(new NullQueryResultTest());
        tests.add(new CrossProcessCursorQueryTest());
        tests.add(new InterprocessBlobQueryTest());
        tests.add(new LoopingQueryTest());
        tests.add(new LoopingCountQueryTest());
        tests.add(new AttachNewDatabaseTest());
        //tests.add(new AttachExistingDatabaseTest());
        tests.add(new CanThrowSQLiteExceptionTest());
        tests.add(new RawExecSQLTest());
        tests.add(new RawExecSQLExceptionTest());
        tests.add(new CompiledSQLUpdateTest());
        //tests.add(new AES128CipherTest());
        //tests.add(new MigrateDatabaseFrom1xFormatToCurrentFormat());
        tests.add(new StatusMemoryUsedTest());
        //tests.add(new ImportUnencryptedDatabaseTest());
        tests.add(new FullTextSearchTest());
        tests.add(new ReadableDatabaseTest());
        tests.add(new AutoVacuumOverReadTest());
        tests.add(new ReadableWritableAccessTest());
        tests.add(new CursorAccessTest());
        tests.add(new VerifyOnUpgradeIsCalledTest());
        //tests.add(new MigrationUserVersion());
        //tests.add(new ExportToUnencryptedDatabase());
        tests.add(new QueryNonEncryptedDatabaseTest());
        tests.add(new EnableForeignKeySupportTest());
        //tests.add(new AverageOpenTimeTest());
        tests.add(new NestedTransactionsTest());
        //tests.add(new ComputeKDFTest());
        //tests.add(new SoundexTest());
        tests.add(new RawQueryTest());
        tests.add(new OpenReadOnlyDatabaseTest());
        tests.add(new RawRekeyTest());
        tests.add(new CorruptDatabaseTest());
        tests.add(new CustomCorruptionHandlerTest());
        tests.add(new MultiThreadReadWriteTest());
        //tests.add(new VerifyUTF8EncodingForKeyTest());
        return tests;
    }
}
