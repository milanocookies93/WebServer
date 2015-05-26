if [ "$EUID" -ne 0 ]
	then echo "Must run as root"
	exit
fi
killall server
./server&
disown
