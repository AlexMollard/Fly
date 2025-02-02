#pragma once

#define M_PI 3.14159265358979323846f
#define M_PI_2 1.57079632679489661923f

#define WIN32_LEAN_AND_MEAN

#include <cmath>
#include <complex>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <tchar.h>
#include <unordered_map>
#include <windows.h>
#include <functional>
#include <sstream>

// External libraries
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

// My files
#include "util/Logger.h"