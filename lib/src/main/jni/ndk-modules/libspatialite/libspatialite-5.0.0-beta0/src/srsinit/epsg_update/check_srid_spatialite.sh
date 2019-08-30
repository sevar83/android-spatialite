#"/bin/bash
# ./check_srid_spatialite.sh
#-------------------------------------------------------------------------------
RED='\033[0;31m';
BRED='\033[1;31m'         # Red
BOLD='\033[1m'  # \x1B[31m
CYAN='\033[0;36m';
BCYAN='\033[1;36m'        # Cyan
BROWN='\033[0;33m';
BBROWN='\033[1;33m'        # Cyan
GREEN='\033[0;32m';
BGREEN='\033[1;32m'       # Green
NC='\033[0m' # No Color
# echo -e "\033[1mThis is bold text.\033[0m"
#-------------------------------------------------------------------------------
BENE="'${GREEN}Tutto bene!${NC}'";
BENENO="'${BRED}No bene!${NC}'";
HABE_FERTIG="${BOLD}Ich habe fertig.${NC}";
BASE_NAME=`basename $0`;
BASE_NAME_CUT=`basename $0 | cut -d '.' -f1`;
MTI="${GREEN}-I->${NC}";
MTW="${BROWN}-W->${NC}";
MTE="${BRED}-E->${NC}";
MESSAGE_TYPE=${MTI};
FORCE_CREATE_DB=0;
VALGRIND_USE=0;
CMD_SPATIALITE=`which spatialite`;
CMD_GDALSRSINFO=`which gdalsrsinfo`;
CMD_SQLITE3=`which sqlite3`;
CMD_VALGRIND=`which valgrind`;
exit_rc=0;
# export "SPATIALITE_SECURITY=relaxed"
#-------------------------------------------------------------------------------
# DateTime functions
# - convert a date into a UNIX timestamp
#  dt_unix_start=$(date2stamp "now");
#  echo "convert a date into a UNIX timestamp [" $dt_unix_start "]";
#-------------------------------------------------------------------------------
date2stamp() 
{
 date --utc --date "$1" +%s
}
#-------------------------------------------------------------------------------
# DateTime functions
# calculate different in seconds from 2 timestamps
#  dt_diff=$(daterun $dt_unix_start $dt_unix_end);
# echo "difference [" $dt_diff "]";
#-------------------------------------------------------------------------------
daterun() 
{
 let diffSec=${2}-${1};
 echo $diffSec;
}
#-------------------------------------------------------------------------------
# DateTime functions
# from from seconds from 2 timestamps
# dt_diff=$(daterun $1 $2);
# echo $(displaytime $dt_diff);
#-------------------------------------------------------------------------------
function displaytime 
{
 local T=$1;
 local D=$((T/60/60/24));
 local H=$((T/60/60%24));
 local M=$((T/60%60));
 local S=$((T%60));
 RESULT="";
 if [ $D -gt 0 ] 
 then
  RESULT="${RESULT}$D days ";
 fi
 if [ $H -gt 0 ] || [ ! -z "${RESULT}" ]
 then
  RESULT="${RESULT}$H hrs ";
 fi
 if [ $M -gt 0 ] || [ ! -z "${RESULT}" ]
 then
  RESULT="${RESULT}$M min ";
 fi
 RESULT="${RESULT}$S sec";
 echo ${RESULT};
}
#-------------------------------------------------------------------------------
# DateTime functions
# calculate different in seconds from 2 timestamps
# echo "run_time [" $(run_time $dt_diff) "]"
#-------------------------------------------------------------------------------
run_time() 
{
 dt_diff=$(daterun $1 $2);
 echo $(displaytime "$dt_diff");
}
#-------------------------------------------------------------------------------
# Start of script
#-------------------------------------------------------------------------------
dt_unix_start=$(date2stamp "now");
#-------------------------------------------------------------------------------
# Adapt db/sql diretory path where needed
#-------------------------------------------------------------------------------
DIR_ERROR_SRID="srid_error";
DB_FILE="check_srid.db";
CSV_FILE="check_srid.csv";
SQL_CHECK="SELECT srid||'#'||srtext||'\n' FROM spatial_ref_sys WHERE srtext <> 'Undefined';";
SQL_FILE="check_srid.sql";
#-------------------------------------------------------------------------------
if [ -f "${DB_FILE}" ]
then
 rm ${DB_FILE};
fi
if [ -f "${CSV_FILE}" ]
then
 rm ${CSV_FILE};
fi
if [  -f "${SQL_FILE}" ]
then
 rm ${SQL_FILE};
fi
#-------------------------------------------------------------------------------
exit_rc=100;
if [ ! -f "${DB_DB_FILE}" ]
then
 if [ ! -f "${SQL_FILE}" ]
 then
  echo ${SQL_CHECK} > ${SQL_FILE};
 fi
 echo -e "${MTI} Database will be created[${DB_PATH}] and a dump of spatial_ref_sys made [${CSV_FILE}].";
 RESULT_TEST=`${CMD_SPATIALITE} ${DB_PATH} < ${SQL_FILE}`;
 if [ ! -z "${RESULT_TEST}" ]
 then
  echo -e ${RESULT_TEST} > ${CSV_FILE};
 else
  echo -e "${MTE} db-creation failed rc[${DB_PATH}]";
 fi
 if [ -f "${CSV_FILE}" ]
 then
  exit_rc=0;
 fi
fi
#-------------------------------------------------------------------------------
count_line=0;
count_srid=0;
count_srid_error=0;
count_srid_ok=0;
declare -a ARRAY_SRID=( );
declare -a ARRAY_SRTEXT=( );
OUTPUT_FILE_SRID="srid_check.txt";
if [ "$exit_rc" -eq "0" ]
then
 while read line
 do
  # line=${CONFIG_FILE_READ_ARRAY[$Config_File_Line_Nr]}
  ((count_line++));
  IFS_SAVE=$IFS;
  IFS='#'
  INPUT_STR_ARRAY=( ${line} );
  IFS=$IFS_SAVE;
  SRID_TEXT="${INPUT_STR_ARRAY[0]}";
  SRTEXT_TEXT="${INPUT_STR_ARRAY[1]}";
  if [ ! -z "${SRID_TEXT}" ] && [ ! -z "${SRTEXT_TEXT}" ] 
  then
   ((count_srid++));
   echo -e ${SRTEXT_TEXT} > ${OUTPUT_FILE_SRID};
   if [ -f "${OUTPUT_FILE_SRID}" ]
   then
    RESULT_TEST=`${CMD_GDALSRSINFO} ${OUTPUT_FILE_SRID} 2>&1`;
    exit_rc=$?
    # echo "-I-> result_text[${RESULT_TEST}]";
    if [ ! -z "${RESULT_TEST}" ]
    then
     srid_rc=0;
     case ${RESULT_TEST} in ERROR*)
      srid_rc=1;
     esac
     if [ "$srid_rc" -eq "0" ]
     then
      echo -e "${MTI} gdalsrsinfo for srid[${SRID_TEXT}] rc[${srid_rc}] ";
      ((count_srid_ok++));
     else
      ((count_srid_error++));
      if [ ! -d "$DIR_ERROR_SRID" ]
      then
       echo -e "${MTW} creating srid-error directory[${DIR_ERROR_SRID}]";
       mkdir $DIR_ERROR_SRID
      fi
      OUTPUT_FILE_SRID_ERROR="${DIR_ERROR_SRID}/srid_${SRID_TEXT}.txt";
      echo -e ${SRTEXT_TEXT} > ${OUTPUT_FILE_SRID_ERROR};
      echo -e "${MTE} srid[${SRID_TEXT}] srtext['${SRTEXT_TEXT}'] ";
     fi
    fi
   fi
  fi
  #------------------------------------------------------------------------------
  line_prev=$line;
  #------------------------------------------------------------------------------
 done < "${CSV_FILE}";
 #------------------------------------------------------------------------------
fi
#-------------------------------------------------------------------------------
echo -e "${MTI} removing work files (database, sql, csv and srid-output)";
if [  -f "${DB_DB_FILE}" ]
then
 rm ${DB_DB_FILE}M
fi
if [ -f "${SQL_FILE}" ]
then
 rm ${SQL_FILE};
fi
if [ -f "${CSV_FILE}" ]
then
 rm ${CSV_FILE};
fi
if [ -f "${OUTPUT_FILE_SRID}" ]
then
 rm ${OUTPUT_FILE_SRID};
fi
#-------------------------------------------------------------------------------
if [ "$count_srid_error" -eq "0" ]
then
 exit_rc=0;
 echo -e "${MTI} gdalsrsinfo checked ${count_srid} srids. correct[${count_srid_ok}] errors[${count_srid_error}] ";
else
 exit_rc=10;
 echo -e "${MTE} gdalsrsinfo checked ${count_srid} srids. correct[${count_srid_ok}] errors[${count_srid_error}] ";
fi
#-------------------------------------------------------------------------------
dt_unix_end=$(date2stamp "now");
dt_run=$(run_time $dt_unix_start $dt_unix_end)
if [ "$exit_rc" -eq "0" ]
then
 RC_TEXT=$BENE;
 TIME_RUN="time_run[${GREEN}$dt_run${NC}]"
else
 TIME_RUN="time_run[${BRED}$dt_run${NC}]"
 RC_TEXT="$BENENO";
 MESSAGE_TYPE=${MTE};
fi
echo -e "${MESSAGE_TYPE} ${BASE_NAME_CUT} rc=$exit_rc - ${TIME_RUN} - [${RC_TEXT}] - ${HABE_FERTIG}";
exit $exit_rc;
#-------------------------------------------------------------------------------

