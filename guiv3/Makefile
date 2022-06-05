GCCF = gcc -Wall -ggdb -O0

.PHONY: prepare
prepare:
	sudo dnf install gtk2-devel gstreamer1-devel clutter-devel webkit2gtk2-devel libgda-devel gobject-introspection-devel
	sudo dnf install devhelp gtk2-devel-docs gstreamer1-devel-docs clutter-doc

.PHONY: build
build:
	gcc -g -Wall simple_gui.c -o simple_gui `pkg-config --cflags --libs gtk+-2.0`

.PHONY: run
run:
	./p1

.PHONY: clean
clean:
	rm p1 simple_gui.o
