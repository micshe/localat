CFLAGS="-Wall"
PREFIX=/usr/local
DESTDIR=

help: private-help options

private-help:
	@echo "liblocalat makefile help"
	@echo "************************" 
	@echo "to view this helpfile, type"
	@echo " $$ make help" 
	@echo "or just"
	@echo " $$ make"
	@echo
	@echo "to just view the configuration options, type"
	@echo " $$ make options"
	@echo
	@echo "makefile targets"
	@echo "****************"
	@echo "to build a shared library, type"
	@echo " $$ make shared"
	@echo
	@echo "to build a static library, type"
	@echo " $$ make static"
	@echo
	@echo "to build both shared and static libraries, type"
	@echo " $$ make all" 
	@echo
	@echo "makefile actions"
	@echo "****************" 
	@echo "to test the liblocalat, type"
	@echo " $$ make test"
	@echo 
	@echo "to install the shared library, type"
	@echo " $$ make install-shared"
	@echo 
	@echo "to install the static library, type"
	@echo " $$ make install-static"
	@echo 
	@echo "to install both shared and static libraries, type"
	@echo " $$ make install"
	@echo
	@echo "to uninstall the shared library, type"
	@echo " $$ make uninstall-shared"
	@echo
	@echo "to uninstall the static library, type"
	@echo " $$ make uninstall-static"
	@echo
	@echo "to uninstall both shared and static librares, type"
	@echo " $$ make uninstall"
	@echo
	@echo "    if liblocalat was installed in a custom location"
	@echo ' (by overloading either the $$PREFIX or $$DESTDIR options)'
	@echo " remember to overload these options with the same values"
	@echo "                  when uninstalling" 
	@echo
	@echo "to clean the source directory, type"
	@echo " $$ make clean"

options:
	@echo
	@echo "makefile configuration options"
	@echo "******************************"
	@echo "CC=$(CC)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "PREFIX=$(PREFIX)"
	@echo "DESTDIR=$(DESTDIR)"
	@echo
	@echo 'liblocalat installs into $$DESTDIR$$PREFIX/lib and $$DESTDIR$$PREFIX/include'
	@echo which is currently: $(DESTDIR)$(PREFIX)/lib and $(DESTDIR)$(PREFIX)/include
	@echo 
	@echo "to overload any of these options, type"
	@echo " $$ make ... CC=/alternative/c/compiler PREFIX=/alternative/prefix/path DESTDIR=/alternative/dest/dir"

localat.o: localat.c localat.h
	$(CC) -c localat.c -o localat.o $(CFLAGS)

liblocalat.a: localat.c localat.h
	@echo "building static localat library"
	$(CC) -c localat.c -o localat.o $(CFLAGS)
	$(AR) rcs liblocalat.a localat.o

liblocalat.so: localat.c localat.h
	@echo "building shared localat library"
	$(CC) -c -fPIC localat.c -o localat.o $(CFLAGS)
	$(CC) -shared localat.o -o liblocalat.so

shared: liblocalat.so

static: liblocalat.a

all: shared static

install-header:
	@echo
	@echo installing localat.h into $(DESTDIR)$(PREFIX)/include
	@mkdir -p $(DESTDIR)$(PREFIX)/include
	@cp localat.h $(DESTDIR)$(PREFIX)/include/localat.h
	@chmod 644 $(DESTDIR)$(PREFIX)/include/localat.h

install-shared: install-header shared
	@echo
	@echo installing liblocalat.so into $(DESTDIR)$(PREFIX)/lib
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@cp liblocalat.so $(DESTDIR)$(PREFIX)/lib/liblocalat.so
	@chmod 644 $(DESTDIR)$(PREFIX)/lib/liblocalat.so
	@echo '  to use liblocalat.so in another program'
	@echo '  set CFLAGS="-I$(DESTDIR)$(PREFIX)/include"'
	@echo '  and LDFLAGS="-L$(DESTDIR)$(PREFIX)/lib -llocalat"'
	@echo
	@echo "run 'ldconfig' now to add liblocalat.so to the shared library database."

install-static: install-header static
	@echo
	@echo installing liblocalat.a into $(DESTDIR)$(PREFIX)/lib
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@cp liblocalat.a $(DESTDIR)$(PREFIX)/lib/liblocalat.a
	@chmod 644 $(DESTDIR)$(PREFIX)/lib/liblocalat.a
	@echo '  to use liblocalat.a in another program'
	@echo '  set CFLAGS="-I$(DESTDIR)$(PREFIX)/include"'
	@echo '  and LDFLAGS="-L$(DESTDIR)$(PREFIX)/lib -llocalat"'

install: install-static install-shared
	@echo

uninstall-header:
	@echo
	@echo uninstalling localat.h from $(DESTDIR)$(PREFIX)/include
	@rm -f $(DESTDIR)$(PREFIX)/include/localat.h

uninstall-shared:
	@echo
	@echo uninstalling liblocalat.so from $(DESTDIR)$(PREFIX)/lib
	@rm -f $(DESTDIR)$(PREFIX)/lib/liblocalat.so
	
uninstall-static:
	@echo
	@echo uninstalling liblocalat.a from $(DESTDIR)$(PREFIX)/lib
	@rm -f $(DESTDIR)$(PREFIX)/lib/liblocalat.a

uninstall: uninstall-shared uninstall-static uninstall-header
	@echo

clean:
	@echo
	@echo "cleaning source directory"
	@rm -f localat.o liblocalat.so liblocalat.a test
	@echo

test: test.c localat.c localat.h
	$(CC) localat.c test.c -o test $(CFLAGS)
	@./test
	@echo

.PHONY: private-help options help clean install install-static install-shared all static shared uninstall uninstall-static uninstall-shared test install-header uninstall-header

