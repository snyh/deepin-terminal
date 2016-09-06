slave-deps = glib-2.0

all: pty-filter

vala: $(wildcard *.vala)
	valac -C $^ -d .tmp --pkg gio-2.0

pty-filter: $(wildcard *.cc) 
	g++ -std=c++11 $^ -o $@ `pkg-config --libs --cflags $(slave-deps)` -lutil
