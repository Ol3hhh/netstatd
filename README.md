# Netstat Daemon

A Linux daemon that monitors network traffic on a network interface, counts packets per IP address, stores the statistics in a SQLite database, and provides a simple interface for querying via a UNIX domain socket.

---

## Requirements

- Linux system
- GCC or compatible C compiler
- **libpcap** development libraries
- **SQLite3** development libraries

---

## Running 
- make clean && make
- sudo ./bin/daemon &
  or
- sudo ./bin/cli start

- sudo ./bin/cli stat [interface] (eth0 default, if you don't have  please change maunaly to your default one in daemon.c)
- sudo ./bin/cli select iface [interface]
- sudo ./bin/cli show [ip address]
- sudo ./bin/cli --help
- sudo ./bin/cli stop
