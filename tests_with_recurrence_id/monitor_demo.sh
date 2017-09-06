#!/bin/bash

COLOR='ms=1;100;37'
COMBINED='ms=7;104;30'

while true ; do
	clear;
	./compile_and_run_demo.sh < calendar.ics | \
        GREP_COLORS=$COLOR       grep --color=always --line-buffered --text -e '|[x-]*[0-9a-f]*[x-]*|' -e '' | \
        GREP_COLORS=$COMBINED    grep --color=always --line-buffered --text -e '|\( *[0-9a-f]* *\)*|' -e '' | \
        GREP_COLORS="ms=1;40;39" grep --color=always --line-buffered --text -e 'UTC' -e 'DATE' -e 'DAYLIGHT' -e 'NOT VALID' -e "v_" -e "D_" -e "L_" -e "U_" -e "Europe/Brussels" -e ''
	sleep 1;
done

