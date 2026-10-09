for i in $(seq 1 20); do
    ./cmake-build-debug/EventSend/EventSend /tmp/oslab-events.fifo INFO "message $i" &
done
wait