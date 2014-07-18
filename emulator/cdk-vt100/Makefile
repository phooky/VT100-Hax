TARGET=vt100sim

OBJS=main.o nvr.o keyboard.o vt100sim.o \
	8080/simglb.o \
	8080/sim1a.o \
	8080/simint.o

all: $(TARGET)

$(TARGET): $(OBJS)
	g++ -o $(TARGET) $^ -lcdk -lncurses

clean:
	rm $(OBJS) $(TARGET)


