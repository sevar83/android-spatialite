package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.QueryHelper;

public class ComputeKDFTest extends SQLCipherTest {

    @Override
    public boolean execute(SQLiteDatabase database) {
        database.rawExecSQL("PRAGMA cipher_kdf_compute;");
        String kdf = QueryHelper.singleValueFromQuery(database, "PRAGMA kdf_iter;");
        setMessage(String.format("Computed KDF:%s", kdf));
        return true;
    }

    @Override
    public String getName() {
        return "Compute KDF Test";
    }
}
