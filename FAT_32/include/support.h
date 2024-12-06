#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdbool.h>
#include "fat32.h"

// bool cstr_to_fat16wnull(char *filename, char output[FAT16STR_SIZE_WNULL]);
bool cstr_to_fat32wnull(char *filename, char output[FAT32STR_SIZE_WNULL]);

#endif
