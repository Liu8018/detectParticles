#pragma once

#include <iostream>
#include <array>
#include <vector>

typedef unsigned char BYTE;

__declspec(dllexport) void detectParticles(BYTE* pbuff, const int width, const int height, std::vector<std::array<int, 3>> & particlesInfo, const int minDiameter, const int maxDiameter);