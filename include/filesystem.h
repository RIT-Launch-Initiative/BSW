#pragma once
#include <Arduino.h>

static constexpr size_t FILE_NAME_SIZE = 32;

void filesystemInit();

uint32_t readUintFromFile(const char* path, uint32_t fallback = 0);

bool writeUintToFile(const char* path, uint32_t v);
