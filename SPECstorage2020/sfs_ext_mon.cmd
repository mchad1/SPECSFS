REM
REM %1 is the current phase. INIT, WARMUP, RUNNING, STOP
REM %2 is the current run number.
REM %3 is the maximum run number.
REM %4 is the current load value.
REM %5 are the arguments that the user put in PRIME_MONITOR_ARGS in 
REM    the sfs_rc file
REM
REM
echo %1 %2 %3 %4 %5  >> c:\tmp\my_monitor_out.txt
exit
