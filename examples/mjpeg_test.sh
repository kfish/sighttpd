#!/bin/bash
#
# Motion JPEG test
#
# After running this shell script, open http://localhost:3000/mjpeg/ with
# Firefox. You will need the following in /etc/sighttpd/sighttpd.conf:
#
# -----
# Listen 3000
# 
# <Stdin>
# 	Path "/mjpeg/"
# 	Type "multipart/x-mixed-replace; boundary=++++++++"
# </Stdin>
# -----

if [ $# -lt 1 ]; then
	echo "Usage: $0 jpeg_file [...]"
	exit 1;
fi

count=0
while [ $# -gt 0 ]; 
do
	if [ ! -f $1 ]; then
		echo "File '$1' not fonund."
	else
		FILE[$count]=$1
		SIZE[$count]=`ls -l $1 | awk '{ print $5 }'`
		((count++))
	fi
	shift;
done

PORT=3000
SIGHTTPD=sighttpd
BOUNDARY=++++++++

( 
	while true;
	do
		n=0
		while [ $n -lt $count ];
		do
			printf "\r\n\r\n--%s\r\n" ${BOUNDARY}
			printf "Content-type: image/jpeg\r\n"
			printf "Content-length: %d\r\n\r\n" ${SIZE[$n]}
			cat ${FILE[$n]}
			((n++))
		done;
	done
) | ${SIGHTTPD} ${PORT}
