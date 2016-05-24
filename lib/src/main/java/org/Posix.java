package org;

// Taken from http://stackoverflow.com/a/20437164/458668
public class Posix {
    public static native int setenv(String key, String value, boolean overwrite);
}
