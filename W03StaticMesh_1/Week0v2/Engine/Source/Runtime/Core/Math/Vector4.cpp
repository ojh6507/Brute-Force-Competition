#include "Vector4.h"


const FVector4 FVector4::WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
const FVector4 FVector4::RED = { 1.0f, 0.0f, 0.0f, 1.0f };
const FVector4 FVector4::GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
const FVector4 FVector4::BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
const FVector4 FVector4::BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };

const FVector4 FVector4::ONE = { 1.0f, 1.0f, 1.0f, 1.0f };
const FVector4 FVector4::ONENULL = { 1.0f, 1.0f, 1.0f, 0.0f };
const FVector4 FVector4::ZERO = { 0.0f, 0.0f, 0.0f, 1.0f };
const FVector4 FVector4::ZERONULL = { 0.0f, 0.0f, 0.0f, 0.0f };

const FVector4 FVector4::LEFT = { -1.0f, 0.0f, 0.0f, 0.0f };
const FVector4 FVector4::RIGHT = { 1.0f, 0.0f, 0.0f, 0.0f };
const FVector4 FVector4::UP = { 0.0f, 1.0f, 0.0f, 0.0f };
const FVector4 FVector4::DOWN = { 0.0f, -1.0f, 0.0f, 0.0f };
const FVector4 FVector4::FORWARD = { 0.0f, 0.0f, 1.0f, 0.0f };
const FVector4 FVector4::BACKWARD = { 0.0f, 0.0f, -1.0f, 0.0f };