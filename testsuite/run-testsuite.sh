#! /usr/bin/env bash

UNLOCK_KEY=82
BIN=build/outputs/apk/testsuite-debug.apk
INSTALL_ROOT=/data/data/org.spatialite.testsuite
EMULATOR_CHECK_STATUS="adb shell getprop init.svc.bootanim"
EMULATOR_IS_BOOTED="stopped"
emulators=`android list avd | awk '/Name:/{print $2}' | sort -u`
for emulator in ${emulators}; do
    emulator @${emulator} -no-skin &> /dev/null &
    OUT=$($EMULATOR_CHECK_STATUS 2> /dev/null)
    printf "Booting ${emulator}..."
    while [[ ${OUT:0:7}  != $EMULATOR_IS_BOOTED ]]; do
        OUT=$($EMULATOR_CHECK_STATUS 2> /dev/null)
        printf "."
        sleep 5
    done
    printf "\n"
    
    # unlock
    adb shell input keyevent ${UNLOCK_KEY}

    # launch and run test suite
    printf "Installing test suite\n"
    adb install -r ${BIN} &> /dev/null

    printf "Running test suite..."
    adb shell am start -n "org.spatialite.testsuite/org.spatialite.testsuite.TestSuiteActivity" \
        -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -e run 1 &> /dev/null

    # remove previous test results
    adb shell rm ${INSTALL_ROOT}/files/test-results.log &> /dev/null

    #poll for test results
    adb pull ${INSTALL_ROOT}/files/test-results.log test-results-$emulator.log &> /dev/null
    OUT=$?
    printf "."
    while [[ ${OUT} != 0 ]]; do
        sleep 5
        adb pull ${INSTALL_ROOT}/files/test-results.log test-results-$emulator.log &> /dev/null
        OUT=$?
        printf "."
    done
    printf "\nTest suite run complete for ${emulator}:\n"
    cat test-results-$emulator.log
    printf "\n"
    sleep 5
    
    # stop emulator
    adb emu kill
done
