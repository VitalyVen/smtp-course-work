# Diable all make built-in rules
.SUFFIXES:

# Default C compiler
# CLang:
#CC = clang
# GCC:
CC = gcc

# We need C99 (-std=c99) and anonymous structures added in C11.
# -D_GNU_SOURCE required for: strdup, pthread_rwlock_t, fdopen,
# and other functions which are not present in basic POSIX standard
# (--std=gnu99 allows only strdup).
CFLAGS = -std=c99 -D_GNU_SOURCE

# Lets add all basic warnings (-Wall) and some additional
# warnings (-Wextra) except 'unused parameters' warning.
CFLAGS += -Wall -Wextra -Wno-unused-parameter

# "-Werror": any warning will cause compilation error.
# It is a very strict compiling option.
# Good for learning, bad for source release.
CFLAGS += -Werror

# This function check if warning option is supported by compiler.
checkw = echo "" | $(CC) -Werror -W$1 -xc -c -o /dev/null - 2>/dev/null && echo ok

# Format strings should be constant
ifeq ($(shell $(call checkw,format-security)), ok)
    CFLAGS += -Wformat-security
endif

# '-lpthread' links pthread library.
STDLIBS = -lpthread

SYSTEM := $(shell uname -s)

# GNU/Linux distributions
ifeq ($(findstring Linux, $(SYSTEM)), Linux)
    # BSD systems does not have separate dl library.
    # GNU systems require -ldl for dlopen().
    STDLIBS += -ldl
else
# Other GNU distributions (i.e., GNU/kFreBSD)
ifeq ($(findstring GNU, $(SYSTEM)), GNU)
    STDLIBS += -ldl
else
# BSD systems
ifeq ($(findstring BSD, $(SYSTEM)), BSD)
    # Add some sensible include path.
    # We could use C_INCLUDE_PATH environment variable too.
    OPT += -I/usr/include -I/usr/local/include
    # Set some sensible library path.
    # (BTW, clang 3.0 does not honour LIBRARY_PATH variable)
    LIBPATH = -L/usr/lib -L/usr/local/lib
endif
endif
endif

# A shared library should contain position-independent code (PIC).
# GCC option '-fPIC' produces Position Independent Code.
# The following line turn it on.
OPT += -fPIC

# Debug build:
# $ make mode=debug
ifeq ($(mode), debug)
    # Lets define DEBUG macro.
    CFLAGS += -DDEBUG
endif

# Reelase build:
# $ make mode=release
ifeq ($(mode), release)
    # Lets define NDEBUG macro and disable all asserts().
    CFLAGS += -DNDEBUG
endif


OBJ = $(SRC:%.c=%.o)

# TODO: EXECOBJ?
ifneq ($(TARGETLIB),)
    EXECOBJ =
else
    EXECOBJ = $(OBJ)
    TESTOBJ = $(OBJ)
endif

ifeq ($(TESTINCD),)
    TESTINCD = $(INCD)
endif

# $(LIBS) should contain static libraries list.

# -iquote ../libdir1/inc -iquote../libdir2/inc etc.:
INCD += $(addsuffix inc, $(dir $(LIBS)))

ifeq ($(TESTLIBS),)
    TESTLIBS = $(LIBS)
endif

$(TARGETLIB): $(OBJ)
	$(RM) $@
	ar rcs $@ $(OBJ)

# $(INCD) should contain all include files directories:
INC = $(addprefix -iquote, $(INCD))
H = $(wildcard $(addsuffix /*.h, $(INCD)))

TESTINCD += $(addsuffix inc, $(dir $(TESTLIBS)))
TESTINC = $(addprefix -iquote, $(TESTINCD))
TESTH = $(wildcard $(addsuffix /*.h, $(TESTINCD)))

# Each .o file depends on appropirate .c file.
# For the sake of simplicity every .o file depends on all header files.
src/%.o: src/%.c $(H)
	$(CC) $(CFLAGS) $(OPT) $(INC) -c $< -o $@

tests/%.o: tests/%.c $(TESTH)
	$(CC) $(CFLAGS) $(TESTINC) -c $< -o $@

# We must statically link TESTLIBS and dynamically link shared libraries.
# '-lcunit' links CUnit library.
tests/run: tests/run.o $(TARGETLIB) $(TESTLIBS) $(TESTOBJ)
	$(CC) $< $(TESTOBJ) $(TARGETLIB) $(TESTLIBS) \
	      -Wl,-Bdynamic $(LIBPATH) -lcunit $(STDLIBS) -o $@

# Target executable rule.
$(TARGETEXEC): src/main.o $(TARGETLIB) $(LIBS) $(EXECOBJ)
	$(CC) $< $(EXECOBJ) $(TARGETLIB) $(LIBS) \
	      -Wl,-Bdynamic $(LIBPATH) $(STDLIBS) -o $@

# Each libsome.so library requires appropirate some.o file.
lib%.so: src/%.o $(LIBS)
	$(CC) $^ -shared -o $@

.PHONY: clean
clean:
# Delete all executable files.
	-find ./ -perm +111 -type f -print | xargs $(RM)
# Delete object files, libraries, and log files.
	$(RM) */*.o *.a *.so *.log

.PHONY: tests

# Adding tests/run dependency
all: tests/run

## NODEPLOY: code style.
STYLE = --brackets=linux --indent=spaces=4
STYLE += --delete-empty-lines
STYLE += --align-pointer=name
STYLE += --break-closing-brackets

.PHONY: style
ALLC := $(wildcard */*.c)
ALLH := $(wildcard */*.h)
style: $(ALLC) $(ALLH)
# reverse: sed ':a;N;$!ba;s/\n\([_a-zA-Z0-9][ _a-zA-Z0-9\*]*\)\n/\n\1 /g' -i $(ALLC)
	sed "s/^\([_a-zA-Z0-9][ _a-zA-Z0-9]\+[ \*]\)\+\([_a-zA-Z0-9]\+(.*).*\)$$/\1\n\2/" -i $(ALLC)
	astyle $(STYLE) -n $^
	sed "s/^\([_a-zA-Z0-9]\+(.*)\)[ ]\({\)$$/\1\n\2/" -i $(ALLC)
## NODEPLOY
