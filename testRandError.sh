#!/bin/bash

# Usage: $0 [port] [infile] [rate]
if [ $# -ne 3 ]; then
    echo "Usage: $0 [DESIRED SERVER PORT] [FILE_IN] [ERR RATE]"
    exit 2
fi

# ===============================

APP_SERVER=server
APP_CLIENT=rcopy

PORT=$1

FILE=$2
FILEOUT=`basename $FILE`.tmp
SIZE=1000
ERROR=0.9 # ignored
WIN=      # Leave blank if non-windowed
SERVER=localhost

export CPE464_OVERRIDE_DEBUG=4
export CPE464_OVERRIDE_ERR_RATE=$3
export CPE464_OVERRIDE_ERR_DROP=-1
export CPE464_OVERRIDE_ERR_FLIP=-1

export CPE464_OVERRIDE_PORT=$PORT
export CPE464_OVERRIDE_SEEDRAND=10
echo "========== SERVER ==========="
cp -f $APP_SERVER ${APP_SERVER}_$PORT
APP_SERVER=${APP_SERVER}_$PORT
./$APP_SERVER 0.9 2>&1 | sed 's/.*/                              &/' &

sleep 1

echo "========== CLIENT ==========="
export CPE464_OVERRIDE_PORT=
export CPE464_OVERRIDE_SEEDRAND=2
./$APP_CLIENT $FILE $FILEOUT $SIZE $ERROR $WIN $SERVER $PORT
APP_CLIENT_RES=$?

echo "========== RESULTS ==========="

if [ $APP_CLIENT_RES -ne 0 ]; then
    echo "- Client Returned $APP_CLIENT_RES"
fi

SERV_PID=`/sbin/pidof $APP_SERVER`
if [ `echo $SERV_PID | wc -w` -eq 0 ]; then
	echo "- Server closed early"
elif [ `echo $SERV_PID | wc -w` -gt 1 ]; then
	echo "- Waiting for Server children to close"
    for i in {1..10}; do
        if [ `echo $SERV_PID | wc -w` -gt 1 ]; then
            echo "."
            sleep 1
            SERV_PID=`/sbin/pidof $APP_SERVER`
        else
            break;
        fi
    done
    
    if [ `echo $SERV_PID | wc -w` -gt 1 ]; then
        echo "-- Children didn't close"
        ps -eaf | grep $APP_SERVER | grep -v grep | sed 's/.*/--- &/'
    fi
fi
   
if [ `echo $SERV_PID | wc -w` -gt 0 ]; then
    kill $SERV_PID
	echo "- Waiting for Server to close"
    for i in {1..3}; do
        if [ `echo $SERV_PID | wc -w` -gt 0 ]; then
            echo "."
            sleep 1
            SERV_PID=`/sbin/pidof $APP_SERVER`
        else
            break;
        fi
    done

    if [ `echo $SERV_PID | wc -w` -gt 0 ]; then
	    echo "- Server Didn't Successfully Close"
        kill -s KILL $SERV_PID
    fi
fi

echo "========== DIFF (IN | OUT) ==========="
diff -qs $FILE $FILEOUT
RETVAL=$?

rm -f $FILEOUT
rm -f $APP_SERVER

exit $RETVAL
