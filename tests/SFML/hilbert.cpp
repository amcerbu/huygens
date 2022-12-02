#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

#include "../../src/audio.h"
#include "../../src/delay.h"
#include "../../src/filter.h"

#include "../../src/fourier.h"

#define CHANS 2
#define INPUT 1

#define WIDTH 800 * 2
#define HEIGHT 600 * 2
#define RADIUS 100

#define DARKNESS 255
#define LINEALPHA 255
#define ALPHA 127
#define LIGHTNESS 0

using namespace soundmath;
#define BSIZE 64
#define FRAMERATE 60

// #define N 65536
// #define N 16384
#define N 4096
// #define N 512
#define LAPS 64

inline int f_process(const std::complex<double>* in, std::complex<double>* out)
{
	for (int i = 0; i < N / 2; i++)
	{
		out[i] = in[i];
	}
	for (int i = N / 2; i < N; i++)
	{
		out[i] = 0;
	}

	return 0;
}

Fourier F = Fourier(f_process, N, LAPS);

const int waveSize = SR / FRAMERATE;
sf::Vertex waveform[2 * waveSize];
int waveOrigin = 0;
double gain = 5;

bool ready = false;
bool flipped = false;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		double the_sample = in[CHANS * i + INPUT];
		F.write(the_sample);

		double real, imag;
		F.read(&real, &imag);
		out[i] = imag; // just so we can hear it

		waveform[waveOrigin + waveSize * flipped] = 
			sf::Vertex(
				sf::Vector2f(
					(1 + gain * real) / 2 * WIDTH,
					(1 + gain * imag) / 2 * HEIGHT), 
				sf::Color(LIGHTNESS,LIGHTNESS,LIGHTNESS,LINEALPHA)
				);

		waveOrigin--;
		if (waveOrigin < 0)
		{
			ready = true;
			flipped = !flipped;
			waveOrigin += waveSize;
		}
	}

	return 0;
}

Audio A = Audio(process, BSIZE);


int main()
{
	sf::ContextSettings settings;
	settings.antialiasingLevel = 1;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT),
		"Hilbert",
		sf::Style::Titlebar | sf::Style::Close, settings);

	window.setFramerateLimit(FRAMERATE);

	sf::VertexArray triangle(sf::Quads, 4);

	// define the position of the triangle's points
	triangle[0].position = sf::Vector2f(0, 0);
	triangle[2].position = sf::Vector2f(WIDTH, HEIGHT);
	triangle[1].position = sf::Vector2f(0, HEIGHT);
	triangle[3].position = sf::Vector2f(WIDTH, 0);

	// define the color of the triangle's points
	triangle[0].color = sf::Color(255, 255, 255, ALPHA); // sf::Color(DARKNESS, 0, 0, ALPHA); //::Red;
	triangle[1].color = sf::Color(255, 255, 255, ALPHA); // sf::Color(0, 0, DARKNESS, ALPHA); //::Bue;
	triangle[2].color = sf::Color(255, 255, 255, ALPHA); // sf::Color(0, DARKNESS, 0, ALPHA); //::Green;
	triangle[3].color = sf::Color(255, 255, 255, ALPHA); // sf::Color(DARKNESS, DARKNESS, 0, ALPHA); //::Yellow;

	window.clear(sf::Color::White);

	A.startup(CHANS, 1, true); // startup audio engine; 4 inputs, 1 outputs, console output on

	while (window.isOpen())
	{
		// event polling
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
					window.close();

				else if (event.key.code == sf::Keyboard::Left)
				{
				}
				else if (event.key.code == sf::Keyboard::Right)
				{
				}
				else if (event.key.code == sf::Keyboard::Down)
				{
					gain -= 0.05;
				}
				else if (event.key.code == sf::Keyboard::Up) 
				{
					gain += 0.05;
				}
			}
			else if (event.type == sf::Event::KeyReleased)
			{
				if (event.key.code == sf::Keyboard::Left)
				{
				}
				if (event.key.code == sf::Keyboard::Right)
				{
				}
				if (event.key.code == sf::Keyboard::Up)
				{
				}
				if (event.key.code == sf::Keyboard::Down)
				{
				}
			}
		}
		

		// window.clear(sf::Color(0,0,0,10));
		
		window.draw(triangle);
		
		while (!ready)
		{
			sf::sleep(sf::microseconds(100));
		}
		window.draw(waveform + waveSize * (!flipped), waveSize, sf::LineStrip);
		ready = false;

		window.display(); // window is done drawing
	}

	A.shutdown(); // shutdown audio engine

	return 0;
}