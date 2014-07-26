import serial

s = serial.Serial('/dev/ttyACM0')
s.flushInput()

def get_ready():
    s.timeout = 5
    rl = s.readline().strip()
    if rl[-5:] != "READY":
        # Attempt to get a ready message by sending a bad command
        s.write(' ')
        s.flush()
        rl = s.readline().strip()
        sys.stderr.write(":forcing:\n")
        if rl == "ERR":
            rl = s.readline().strip()
        if rl != "READY":
            raise RuntimeError("READY message not received, got"+rl)
    s.timeout = None

def read_rom_raw():
    get_ready()
    s.write('R')
    lines = []
    while True:
        l = s.readline().strip()
        if l.strip() == 'ERR':
            raise RuntimeError("Unexpected read error")
        elif l.strip() == 'OK':
            return lines
        else:
            lines.append(l)

def parse_line(line):
    parts = line.split()
    addr = int(parts[0],16)
    data = ''
    for b in parts[1:17]:
        data = data + chr(int(b,16))
    return (addr,data)
    
def read_rom():
    data = ''
    lines = read_rom_raw()
    for (addr, d) in map(parse_line,lines):
        if len(data) == addr:
            data = data + d
        else:
            end = addr+len(d)
            if end > len(data):
                data = data + '\x00'*(end-len(data))
            data[addr:addr+len(d)] = d
    return data

def write_rom(data):
    get_ready()
    if len(data) != 8192:
        raise RuntimeError("Incorrect data size; must be 8K")
    s.write('W')
    s.write(data)
    s.flush()
    if s.readline().strip() != 'OK':
        raise RuntimeError("Failed to receive OK from burner")

import sys

if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.stdout.write(read_rom())
        sys.exit(0)
    path = sys.argv[1]
    f = open(path,"rb")
    data = f.read()
    f.close()
    sys.stdout.write("Read {0} bytes of data from {1}.\n".format(len(data),path))
    sys.stdout.write("Writing...")
    sys.stdout.flush()
    write_rom(data)
    sys.stdout.write("done.\n")
    sys.stdout.write("Reading...")
    sys.stdout.flush()
    d2 = read_rom()
    sys.stdout.write("done.\n")
    if (data == d2):
        sys.stdout.write("Data verified.\n")
    else:
        sys.stderr.write("Data is not consistent!\n")
        sys.exit(1)
    sys.exit(0)

            
