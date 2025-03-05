#pragma once

#include "../lib/hw.h"

void print_string(const char* message="", char end='\n');

void print_uint64(uint64 number=0, char end='\n');

void print_horizontal_line(int length=0, char end='\n');

constexpr int DELETE_KEY = 127;
constexpr int ENTER_KEY  = 13;

char* get_string(int max_length=255);