ifneq ($(shell uname), AmigaOS)
	CC 		= ppc-amigaos-gcc
	DELETE		= rm -f
	STRIP = ppc-amigaos-strip
	AMIGADATE = $(shell date +"%-d.%-m.%Y")
else
	CC 		= gcc
	DELETE		= delete
	STRIP = strip
	AMIGADATE = $(shell date LFORMAT "%-d.%-m.%Y")
endif

NAME = ActivityMeter
OBJS = main.o gui.o timer.o logger.o
DEPS = $(OBJS:.o=.d)

CFLAGS = -Wall -Wextra -O3 -gstabs -D__AMIGA_DATE__=\"$(AMIGADATE)\"

# Dependencies
%.d : %.c
	$(CC) -MM -MP -MT $(@:.d=.o) -o $@ $< $(CFLAGS)

%.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(NAME): $(OBJS) makefile
	$(CC) -o $@ $(OBJS) -lauto

clean:
	$(DELETE) $(OBJS)

strip:
	$(STRIP) $(NAME)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
