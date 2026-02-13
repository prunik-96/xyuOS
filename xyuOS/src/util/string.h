// Added functions for uint32 to string conversion and string concatenation

#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Function to convert uint32 to string
void uint32_to_str(uint32_t value, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "%u", value);
}

// Function to concatenate two strings
char* kstrcat(char* dest, const char* src) {
    return strncat(dest, src, strlen(src));
}

#endif // STRING_H