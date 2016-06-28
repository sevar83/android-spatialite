package org.spatialite.testsuite.tests;

public interface ResultNotifier {
    void send(TestResult result);
    void complete();
}
