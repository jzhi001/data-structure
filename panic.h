//
// Created by zjc on 14/02/19.
//

#ifndef PANIC_H
#define PANIC_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void panic(char *format, ...){
    va_list va;
    va_start(va, format);
    vprintf(format, va);
    va_end(va);

    exit(1);
}

#endif //PANIC_H
