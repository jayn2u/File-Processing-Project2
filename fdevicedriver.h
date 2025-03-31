//
// Created by JIWOONG CHOI on 3/31/25.
//

#ifndef FDEVICEDRIVER_H
#define FDEVICEDRIVER_H

int fdd_read(int ppn, char* pagebuf);

int fdd_write(int ppn, char* pagebuf);

int fdd_erase(int ppn);

#endif //FDEVICEDRIVER_H
