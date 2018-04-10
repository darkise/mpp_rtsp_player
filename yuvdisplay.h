#ifndef YUVDISPLAY_H
#define YUVDISPLAY_H

#include <stdlib.h>
#include <stdint.h>

int initWindow();
int initShader();
void copy_data(uint8_t const* data);
void onDrawFrame();

#endif // YUVDISPLAY_H
