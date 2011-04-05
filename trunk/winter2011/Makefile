PROGS = scripts/kconfig/dumpconf undertaker/undertaker2 rsf2model/rsf2model
TEMPLATED = rsf2model/undertaker-kconfigdump
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
VERSION=$(shell cat VERSION)

all: $(PROGS)

scripts/kconfig/dumpconf: FORCE
	$(MAKE) -f Makefile.kbuild dumpconf

undertaker/undertaker2: FORCE
	$(MAKE) -C undertaker undertaker2

rsf2model/rsf2model: FORCE
	$(MAKE) -C rsf2model

%: %.in
	@echo "Template: $< -> $@"
	@sed "s#@LIBDIR@#${LIBDIR}#g" < $< > $@
	@chmod --reference="$<" $@

clean:
	$(MAKE) -f Makefile.kbuild clean
	$(MAKE) -C undertaker2 clean
	$(MAKE) -C ziz clean
	$(MAKE) -C rsf2model clean
	@rm -f $(TEMPLATED)

docs:
	$(MAKE) -C undertaker2 docs

check:
	$(MAKE) -C undertaker2 $@
	$(MAKE) -C rsf2model $@

install: all $(TEMPLATED)
	@install -d -v $(DESTDIR)$(PREFIX)/bin

	@install -d -v $(DESTDIR)$(LIBDIR)/undertaker 2
	@install -d -v $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker2

	@install -v scripts/kconfig/dumpconf $(DESTDIR)$(LIBDIR)/undertaker2
	@install -v rsf2model/rsf2model $(DESTDIR)$(LIBDIR)/undertaker2
	@install -v undertaker/undertaker-scan-head $(DESTDIR)$(LIBDIR)/undertaker2

	@install -v undertaker/undertaker2 $(DESTDIR)$(PREFIX)/bin
	@install -v rsf2model/undertaker-kconfigdump $(DESTDIR)$(PREFIX)/bin
	@install -v undertaker/undertaker2-linux-tree $(DESTDIR)$(PREFIX)/bin

	@install -v contrib/undertaker.el $(DESTDIR)$(PREFIX)/share/emacs/site-lisp/undertaker2

dist: clean
	tar -czvf ../undertaker2-$(VERSION).tar.gz . \
		--show-transformed-names \
		--transform 's,^./,undertaker-$(VERSION)/,'\
		--exclude=*~ \
		--exclude=*.rsf \
		--exclude=*.model \
		--exclude-vcs \
		--exclude="*nfs*" \
		--exclude="*git*" \
		--exclude=*.tar.gz \
		--exclude="*.html"

undertaker-lcov:
	$(MAKE) -C undertaker2 run-lcov

FORCE:
.PHONY: FORCE check undertaker-lcov
