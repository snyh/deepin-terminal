slave-deps = glib-2.0


vala: $(wildcard *.vala)
	valac -C $^ -d .tmp --pkg gio-2.0

slave-binary: $(wildcard *.cc) $(wildcard .tmp/*.c)
	g++ -std=c++98 $^ -o slave `pkg-config --libs --cflags $(slave-deps)` -lutil
