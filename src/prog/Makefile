# 2017-12-11 - Added object file for noise1234.c
# 2017-12-13 - Added object file for python.c
HDRS = ../hdrs
LIB  = libprog.a
OBJS = boolexp.o ctrl.o eval.o hash.o noise1234.o python.o

include ../config.in

$(LIB): $(OBJS)
	rm -f $(LIB)
	ar qcs $(LIB) $(OBJS)

clean:
	rm -f $(LIB) $(OBJS)
