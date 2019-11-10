cc = cc
cflags = -g -DTEST_CHECK_OPTN `autoopts-config cflags`
ldflags = `autoopts-config ldflags`
objects = server.o server-fsm.o server-cmd.o server-parse.o server-run.o server-state.o checkoptn.o
executable = server

checkoptn.c: checkoptn.def
	autogen checkoptn.def

server-fsm.c: server.def
	autogen server.def

checkoptn.o: checkoptn.c
	$(cc) -o checkoptn.o $(cflags) -c checkoptn.c

#server.o: server.c checkoptn.c
#	$(cc) -o $@ -c $<

%.o: %.c checkoptn.c
	$(cc) -o $@ -c $<

$(executable): $(objects)
	cc -o $@ $(objects) $(ldflags)

clean:
	rm -f *.o; \
	rm -f $(executable); \
	rm -f checkoptn.c checkoptn.h;

test:
	./$(executable) -p 1024 -f test01.cmds
	
