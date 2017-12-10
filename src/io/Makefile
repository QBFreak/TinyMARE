# 2017-12-10 - Added sqlite object - QBFreak@qbfreak.net
HDRS = ../hdrs
LIB  = libio.a
OBJS = console.o dns.o file.o html.o ident.o mail.o net.o server.o sqlite.o user.o

include ../config.in

$(LIB): $(OBJS)
	rm -f $(LIB)
	ar qcs $(LIB) $(OBJS)

clean:
	rm -f $(LIB) $(OBJS)
