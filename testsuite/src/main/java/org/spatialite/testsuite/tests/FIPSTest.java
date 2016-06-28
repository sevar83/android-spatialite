package org.spatialite.testsuite.tests;

import org.spatialite.database.SQLiteDatabase;
import org.spatialite.testsuite.QueryHelper;

public class FIPSTest extends SQLCipherTest {
    @Override
    public boolean execute(SQLiteDatabase database) {

        String version = QueryHelper.singleValueFromQuery(database, "PRAGMA cipher_version;");
        setMessage(String.format("SQLCipher version:%s", version));
        int expectedValue = version.contains("FIPS") ? 1 : 0;

        int status = QueryHelper.singleIntegerValueFromQuery(database, "PRAGMA cipher_fips_status;");
        return status == expectedValue;
    }

    @Override
    public String getName() {
        return "FIPS Test";
    }
}
