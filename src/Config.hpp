#pragma once

#include <cstdint>
#include <string>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

inline bool clothSpinning = false;
inline int flipGrav = 1;


const int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t GRID_SIZE_X = 50;
const uint32_t GRID_SIZE_Y = 50;

const std::string MODEL_PATH = "../resources/models/clothplane.obj";
//const std::string MODEL_PATH = "../resources/models/sphereWTex.obj";

const std::string TEXTURE_PATH = "../resources/textures/quilt.jpg";
//const std::string TEXTURE_PATH = "../resources/textures/horse.png";
//const std::string TEXTURE_PATH = "../resources/textures/vox.png";
//const std::string TEXTURE_PATH = "../resources/textures/cp.png";


