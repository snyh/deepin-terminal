slave-deps = glib-2.0 gio-2.0

all: pty-filter

dbus.vala.o: dbus.vala
	valac -c $^ --pkg gio-2.0

pty-filter: $(wildcard *.cc)  dbus.vala.o
	g++ -std=c++11 $^ -o $@ `pkg-config --libs --cflags $(slave-deps)` -lutil
