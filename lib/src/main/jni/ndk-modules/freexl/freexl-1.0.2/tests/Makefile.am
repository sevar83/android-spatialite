check_PROGRAMS = open_oocalc95 \
		open_oocalc97 \
		check_oocalc95 \
		check_oocalc97 \
		open_excel2003 \
		check_excel2003_biff2 \
		check_excel2003_biff3 \
		check_excel2003_biff3_info \
		check_excel2003_biff3_error_checks \
		check_excel2003_biff4 \
		check_excel2003_biff4_workbook \
		check_excel2003_biff5_workbook \
		check_excel2003_biff8 \
		check_excel2003_biff4_1904 \
		walk_fat_oocalc97 \
		walk_sst_oocalc97 \
		check_datetime_biff8 \
		check_boolean_biff8 \
		check_oocalc97_intvalue

AM_CFLAGS = -I@srcdir@/../headers
AM_LDFLAGS = -L../src -lfreexl -lm $(GCOV_FLAGS)

TESTS = $(check_PROGRAMS)

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = testdata/oocalc_empty95.xls \
       testdata/oocalc_empty97.xls \
       testdata/oocalc_simple95.xls \
       testdata/oocalc_simple97.xls \
       testdata/empty2003.xls \
       testdata/simple2003.xls \
       testdata/simple2003_21.xls \
       testdata/simple2003_3.xls \
       testdata/simple2003_4.xls \
       testdata/simple2003_4_1904.xls \
       testdata/simple2003_4WB.xlw \
       testdata/simple2003_5WB.xls \
       testdata/datetime2003.xls \
       testdata/testcase1.xls \
       testdata/testbool.xls \
       test_under_valgrind.sh
