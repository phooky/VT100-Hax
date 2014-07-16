TARGET=vt100sim

OBJS=main.o nvr.o keyboard.o vt100sim.o \
	8080/simglb.o \
	8080/sim1.o \
	8080/sim1a.o \
	8080/sim2.o \
	8080/sim3.o \
	8080/sim4.o \
	8080/sim5.o \
	8080/sim6.o \
	8080/sim7.o \
	8080/simint.o

all: $(TARGET)

$(TARGET): $(OBJS)
	g++ -o $(TARGET) $^

clean:
	rm $(OBJS) $(TARGET)


