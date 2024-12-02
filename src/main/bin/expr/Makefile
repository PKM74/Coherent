DESTDIR=/bin386/bin/
EXPR=$(DESTDIR)expr

CFLAGS=-O
CC=cc

$(EXPR):	expr.o	
	rm -f $(EXPR)
	cc -O -s -o $(EXPR) expr.o -ly
	su root chmog 511 bin bin $(EXPR)

expr.o:	expr.y
