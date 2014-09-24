#pragma once

#include <cstdint>

// XXX: don't use it, it's for internal usage
// r < 32, g < 64, b < 32
static inline uint16_t RGB565_Raw(uint8_t r, uint8_t g, uint8_t b)
{
	return ((uint16_t)r << 11) | ((uint16_t)g << 5) | (uint16_t)b;
}

struct RGB565 {
	uint16_t color;

	RGB565() = default;

	// 0.0f <= v <= 1.0f range expected
	RGB565(float r, float g, float b): color(RGB565_Raw(r * 31.0f, g * 63.0, b * 31.0f)) {}

	float R() const { return (color >> 11) / 31.0f; }
	float G() const { return ((color >> 5) & 63) / 63.0f; }
	float B() const { return (color & 31) / 31.0f; }

	bool operator==(const RGB565 &r) const { return color == r.color; }
	bool operator!=(const RGB565 &r) const { return color != r.color; }

};

static inline RGB565 RGB565_Black()       { return RGB565(0.000, 0.000, 0.000); }
static inline RGB565 RGB565_Grey()        { return RGB565(0.498, 0.498, 0.498); }
static inline RGB565 RGB565_White()       { return RGB565(1.000, 1.000, 1.000); }
static inline RGB565 RGB565_BoldWhite()   { return RGB565(1.000, 1.000, 1.000); }
static inline RGB565 RGB565_Red()         { return RGB565(0.804, 0.000, 0.000); }
static inline RGB565 RGB565_BoldRed()     { return RGB565(1.000, 0.000, 0.000); }
static inline RGB565 RGB565_Green()       { return RGB565(0.000, 0.804, 0.000); }
static inline RGB565 RGB565_BoldGreen()   { return RGB565(0.000, 1.000, 0.000); }
static inline RGB565 RGB565_Blue()        { return RGB565(0.067, 0.235, 0.447); }
static inline RGB565 RGB565_BoldBlue()    { return RGB565(0.192, 0.439, 0.749); }
static inline RGB565 RGB565_Yellow()      { return RGB565(0.804, 0.804, 0.000); }
static inline RGB565 RGB565_BoldYellow()  { return RGB565(1.000, 1.000, 0.000); }
static inline RGB565 RGB565_Magenta()     { return RGB565(0.804, 0.000, 0.804); }
static inline RGB565 RGB565_BoldMagenta() { return RGB565(1.000, 0.000, 1.000); }
static inline RGB565 RGB565_Cyan()        { return RGB565(0.000, 0.804, 0.804); }
static inline RGB565 RGB565_BoldCyan()    { return RGB565(0.000, 1.000, 1.000); }
