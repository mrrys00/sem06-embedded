GCCF = gcc -Wall -ggdb -O0

.PHONY: prepare
prepare:
	sudo dnf install gtk3-devel gstreamer1-devel clutter-devel webkit2gtk3-devel libgda-devel gobject-introspection-devel
	sudo dnf install devhelp gtk3-devel-docs gstreamer1-devel-docs clutter-doc

.PHONY: build
build:
	gcc `pkg-config --cflags gtk4` complex_gui.c -o p1 `pkg-config --libs gtk4`
	# gcc -o p1 simple_gui.c `pkg-config --cflags --libs gtk+-3.0`
	# gcc -o p1 complex_gui.c `pkg-config --cflags --libs gtk+-3.0`
	# $(GCCF) -c simple_gui.c `pkg-config --cflags --libs gtk+-3.0`
	# $(GCCF) -c simple_gui.o -o p1 `pkg-config --cflags --libs gtk+-3.0`
	# chmod +x ./p1

.PHONY: run
run:
	./p1

.PHONY: clean
clean:
	rm p1 simple_gui.o
