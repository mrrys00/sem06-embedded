GCCF = gcc -Wall -ggdb -Ofast
NAME = simple_gui

.PHONY: prepare
prepare:
	sudo dnf install gtk3-devel gstreamer1-devel clutter-devel webkit2gtk3-devel libgda-devel gobject-introspection-devel
	sudo dnf install devhelp gtk3-devel-docs clutter-doc

.PHONY: build
build:
	gcc -g -Wall $(NAME).c -o $(NAME) `pkg-config --cflags --libs gtk+-3.0`

.PHONY: run
run:
	./$(NAME)

.PHONY: all
all:
	make build
	make run

.PHONY: clean
clean:
	rm p1 $(NAME).o
