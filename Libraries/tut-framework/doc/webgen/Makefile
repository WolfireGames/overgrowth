PAGES=$(wildcard src/*.page)
SRC=$(wildcard ../../include/tut/*.hpp)

all: webgen doxygen

webgen: src/default.css src/default.template src/virtual ${PAGES}
	@webgen

doxygen: Doxyfile dox/header.html dox/footer.html dox/doxygen.css dox/main.dox ${SRC}
	@doxygen

clean:
	@rm -rf out tmp
	@rm -f doxywarn.txt
