# Add .d to Make's recognized suffixes.
SUFFIXES += .d

#We don't need to clean up when we're making these targets
NODEPS:=clean clean-check

# DEBUG = -g3 -DDEBUG
# to compile with debug, compile like this:
# make DEBUG="-g3 -DDEBUG"
CFLAGS = -Wall -Wextra -O2 $(DEBUG)
CXXFLAGS = $(CFLAGS)

LDFLAGS =
LDLIBS = librsfmodel.a
PARSEROBJ = KconfigRsfTranslator.o KconfigRsfTranslatorRewrite.o RsfBlocks.o
DEPFILES:=$(patsubst %.o,%.d,$(PARSEROBJ))

PROGS = rsf2model
TESTPROGS = test-KconfigRsfTranslator

all: $(DEPFILES) $(PROGS) $(TESTPROGS)

rsf2model: librsfmodel.a

librsfmodel.a: $(DEPFILES) $(PARSEROBJ)
	ar r $@ $(PARSEROBJ)

#Don't create dependencies when we're cleaning, for instance
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
    #Chances are, these files don't exist.  GMake will create them and
    #clean up automatically afterwards
    -include $(DEPFILES)
endif

#This is the rule for creating the dependency files
%.d: %.cpp
	 @$(CXX) $(CXXFLAGS) -MM $< > $@

clean: clean-check
	rm -rf *.o *.a *.gcda *.gcno *.d

test-%: test-%.cpp librsfmodel.a
	$(CXX) $(CXXFLAGS) -o $@ $< -lcheck $(LDFLAGS) $(LDLIBS)

run-libcheck: $(TESTPROGS)
	@for t in $^; do ./$$t; done

check: all
	@$(MAKE) -s clean-check
	cd validation && ./test-suite
	@$(MAKE) -s run-libcheck

clean-check:
	find validation/ \( -name "*.output.expected" \
                     -o -name "*.output.got" \
                     -o -name "*.output.diff" \
                     -o -name "*.error.expected" \
                     -o -name "*.error.diff" \
                     -o -name "*.error.got" \
                     -o -name "*.dead" \
                     -o -name "*.undead" \
                     \) -delete
docs:
	doxygen

run-lcov:
	$(MAKE) -B DEBUG="-g -O0 -fprofile-arcs -ftest-coverage" LDCOV="-coverage"
	rm -rf coverage-html ; mkdir coverage-html
	lcov --directory $(CURDIR) --zerocounters
	-$(MAKE) check
	lcov --directory $(CURDIR) --capture --output-file coverage-html/undertaker.info
	genhtml -o coverage-html coverage-html/undertaker.info

FORCE:
.PHONY: all clean clean-check FORCE run-% docs validation run-lcov
