// includes.h

#ifndef INCLUDED

#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>

#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>

#include <portaudio.h>
#include "RtMidi.h"

typedef unsigned long ulong;

const double PI = 3.14159265359;
const double E =  2.718281828459045;
const int SR = 48000;
const int FORCE = 50000;
const double A4 = 440.0; // frequency of the A above middle C; tune if necessary

const static double epsilon = std::numeric_limits<double>::epsilon();
const static double order = log2(epsilon);

// return the stiffness coefficient so that relaxation
// occurs in k seconds (for interpolation)
double relaxation(double k)
{
	if (k == 0)
		return 0; 
	return pow(2.0, order / (fmax(0, k) * SR));
}

// midi to frequency
double mtof(double midi)
{
	return A4 * pow(2, (midi - 69) / 12);
}

double ftom(double frequency)
{
	return 69 + log2(frequency / A4) * 12;
}

double atodb(double amplitude)
{
	return 20 * log(amplitude);
}

double dbtoa(double db)
{
	return pow(10, db / 20);
}

std::string notename(int midi)
{
	static std::string notes = "C C#D D#E F F#G G#A A#B ";
	return notes.substr(2 * (midi % 12), 2) + std::to_string(midi / 12 - 1);
}

template <typename T> int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

#endif
#define INCLUDED