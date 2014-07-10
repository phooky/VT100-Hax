#ifndef NVR_H
#define NVR_H

#include <qglobal.h>

class NVR
{
    // The non-volatile ram represents a 14x100 bit array.
public:
    NVR();
    void set_latch(quint8 latch);
    bool output();
    void clock();
    //void load(char* path);
    //void save(char* path);
private:
    quint32 address_reg;
    quint16 data_reg;
    quint16 contents[100];
    quint8 latch;
    bool out;
};

#endif // NVR_H
