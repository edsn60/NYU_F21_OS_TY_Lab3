//
// Created by Ysw on 2021/10/31.
//
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


unsigned char *encoding(char *content){
    size_t size = strlen(content);
    unsigned char *result = (unsigned char*) malloc(sizeof(unsigned char) * (size * 2 + 1));
    if (!result){
        fprintf(stderr, "Error: malloc failed in encoder.c:12\n");
        exit(-1);
    }
    int idx = 0;
    if (!content || strlen(content) == 0){
        result[0] = '\0';
        return result;
    }
    char current_char = *content;
    int count = 0;
    for (char *c = content; *c; c++){
        if (*c == current_char){
            count++;
        }
        else{
            result[idx++] = toascii(current_char);
            result[idx++] = (char)count;
            current_char = *c;
            count = 1;
        }
    }
    result[idx++] = current_char;
    result[idx++] = (char)count;
    result[idx] = '\0';
    return result;
}