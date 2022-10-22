#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

#include "../../src/audio.h"
#include "../../src/delay.h"
#include "../../src/synth.h"
#include "../../src/filter.h"
#include "../../src/metro.h"

#include "../../src/argparse.h"

#define WIDTH 800 * 2
#define HEIGHT 600 * 2
#define RADIUS 100

#define DARKNESS 255
#define ALPHA 127
#define LINEALPHA 255
#define LIGHTNESS 0

using namespace soundmath;
#define BSIZE 64
#define FRAMERATE 60

int CHANS;
int INPUT;
int DEVICE;

void args(int argc, char *argv[])
{
	argparse::ArgumentParser program("oscilloscope");

	int def_in = 0;
	int def_out = 0;
	Audio::initialize(false, &def_in, &def_out);

	program.add_argument("-i", "--input")
		.default_value<int>((int)def_in)
		.required()
		.scan<'i', int>()
		.help("device id for audio in");

	program.add_argument("-f", "--framesize")
		.default_value<int>(1)
		.required()
		.scan<'i', int>()
		.help("channels per frame");

	program.add_argument("-c", "--channel")
		.default_value<int>(0)
		.required()
		.scan<'i', int>()
		.help("input channel");

	program.add_argument("-d", "--devices")
		.help("list audio device names")
		.default_value(false)
		.implicit_value(true);

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		std::exit(1);
	}

	DEVICE = program.get<int>("-i");
	CHANS = program.get<int>("-f");
	INPUT = program.get<int>("-c");
	Audio::initialize(program.is_used("-d"));
}



const int waveSize = SR / FRAMERATE;
sf::Vertex waveform[2 * waveSize];
int waveOrigin = 0;
double gain = 5;
int dtime = 2000;

Delay<double> chandelay(1, SR);

bool ready = false;
bool flipped = false;

inline int process(const float* in, float* out)
{
	for (int i = 0; i < BSIZE; i++)
	{
		out[i] = 0;
		double the_sample = in[CHANS * i + INPUT];
		waveform[waveOrigin + waveSize * flipped] = 
			sf::Vertex(
				sf::Vector2f(
					(1 + gain * the_sample) / 2 * WIDTH,
					(1 + gain * chandelay(the_sample)) / 2 * HEIGHT), 
				sf::Color(LIGHTNESS,LIGHTNESS,LIGHTNESS,LINEALPHA)
				);

		waveOrigin--;
		if (waveOrigin < 0)
		{
			ready = true;
			flipped = !flipped;
			waveOrigin += waveSize;
		}

		chandelay.tick();
	}

	chandelay.coefficients({{dtime,1}},{});

	return 0;
}

Audio A = Audio(process, BSIZE);


int main(int argc, char *argv[])
{
	args(argc, argv);

	sf::ContextSettings settings;
	settings.antialiasingLevel = 1;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT),
		"Oscilloscope",
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

	chandelay.coefficients({{dtime,1}},{});
	A.startup(CHANS, 1, true, DEVICE); // startup audio engine; 4 inputs, 1 outputs, console output on

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
					dtime += 1;
					dtime = std::min(dtime, SR - 1);
				}
				else if (event.key.code == sf::Keyboard::Right)
				{
					dtime -= 1;
					dtime = std::max(100, dtime);
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
		}
		
		
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