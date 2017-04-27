#!/bin/bash

# if all of the reader nodes die, kill runsv causing the container to exit
if [[ "$USE_MULTICORE_READONLY" ]]; then
  STEEMD_READONLY_PIDS=`pgrep -f read-only`
  if [[ ! $? -eq 0 ]]; then
  	echo NOTIFYALERT! steemd reader nodes have quit unexpectedly, starting a new instance..
    RUN_SV_PID=`pgrep -f /etc/service/steemd`
    kill -9 $RUN_SV_PID
  fi
fi

# if the writer node dies, kill runsv causing the container to exit
STEEMD_PID=`pgrep -f p2p-endpoint`
if [[ ! $? -eq 0 ]]; then
  echo NOTIFYALERT! steemd has quit unexpectedly, waiting for core dump and then starting a new instance..
  while [ ! -e /tmp/core ]; do sleep 0.1; done
  gdb --batch --quiet -ex "thread apply all bt full" -ex "quit" /usr/local/steemd-full/bin/steemd /tmp/core >> /tmp/stacktrace
  STACKTRACE=`cat /tmp/stacktrace`
  echo NOTIFYALERT! steemdsync stacktrace from coredump: $STACKTRACE
  RUN_SV_PID=`pgrep -f /etc/service/steemd`
  kill -9 $RUN_SV_PID
fi

# check on this every 10 seconds
sleep 10
