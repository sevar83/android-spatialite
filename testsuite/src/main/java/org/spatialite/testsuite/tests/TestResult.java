package org.spatialite.testsuite.tests;

public class TestResult {

    private String name;
    private boolean success;
    private String message;
    private String error;

    public TestResult(String name, boolean success){
        this(name, success, "");
    }

    public TestResult(String name, boolean success, String error){
        this.name = name;
        this.success = success;
        this.error = error;
    }

    public void setResult(boolean success){
        this.success = success;
    }

    public String getName() {
        return name;
    }

    public boolean isSuccess() {
        return success;
    }

    public String getError() {return error;}

    @Override
    public String toString() {
        return isSuccess() ? "OK" : "FAILED";
    }


    public void setMessage(String message) {
        this.message = message;
    }

    public String getMessage(){
        return this.message;
    }
}
