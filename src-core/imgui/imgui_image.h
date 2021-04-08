#pragma once

#include "imgui.h"
#include <cstdint>
#include <functional>

extern std::function<unsigned int()> makeImageTexture;
extern std::function<void(unsigned int, uint32_t *, int, int)> updateImageTexture;
extern std::function<void(unsigned int)> deleteImageTexture;

void ushort_to_rgba(uint16_t *input, uint32_t *output, int size, int channels = 1);