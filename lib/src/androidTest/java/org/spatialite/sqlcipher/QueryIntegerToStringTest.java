package org.spatialite.sqlcipher;

import org.junit.Test;

import static junit.framework.Assert.assertEquals;

public class QueryIntegerToStringTest extends BaseTest {
    @Test
    public void queryIntegerToString() {
        String value = QueryHelper.singleValueFromQuery(database, "SELECT 123;");
        assertEquals("123", value);
    }
}
