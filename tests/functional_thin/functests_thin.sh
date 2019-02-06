#!/bin/bash

clinode=${CLINODE:-./clinode}
clinode_conf=${CLINODE_CONF:-functests_thin_clinode.cfg}
functests=${FUNCTESTS_THIN:-./functests_thin}
functests_conf=${FUNCTESTS_THIN_CONF:-functests_thin.cfg}


run_server() {
    $clinode -c $clinode_conf &
    if [ $? -ne 0 ]; then
        echo "ERROR: $clinode returned non-zero exit status"
        exit 1
    fi
}

kill_server() {
    echo "Killing $clinode"
    kill $!
    if [ $? -ne 0 ]; then
        echo "ERROR: pkill returned non-zero exit status"
        exit 1
    fi
}

run_functests_thin() {
    $functests -c $functests_conf
    if [ $? -ne 0 ]; then
        echo "ERROR: $functests returned non-zero exit status"
        kill_server
        exit 1
    fi
}

if [ ! -f "$clinode" ] && [ ! $(command -v "$clinode") ]; then
    echo "No such file or directory: $clinode"
    exit 1
fi

if [ ! -f "$functests" ] && [ ! $(command -v "$functests") ]; then
    echo "No such file or directory: $functests"
    exit 1
fi

trap kill_server SIGINT SIGTERM

echo "Starting server"
run_server

sleep 3

echo "Executing tests"
run_functests_thin

kill_server