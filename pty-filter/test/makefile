test: $(wildcard *.cc) ../context.cc ../global.cc ../io.cc ../mypty.cc
	g++ -o $@ $^ `pkg-config --libs --cflags cpputest glib-2.0` -I.. && ./test
