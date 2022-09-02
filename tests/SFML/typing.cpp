// #include "../src/audio.h"
#include <iostream>
#include <string>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

// #include "../../src/delay.h"
// #include "../../src/filter.h"
#include "../../src/includes.h"
#include "../../src/midi.h"

#define WIDTH 400
#define HEIGHT 300

using namespace soundmath;


std::map<char, unsigned char> keymapping = {{'z', 40}, {'x', 41}, {'c', 42}, {'v', 43}, {'b', 44}, {'n', 45}, 
								      {'m', 46}, {',', 47}, {'.', 48}, {'/', 49},
								  {'a', 45}, {'s', 46}, {'d', 47}, {'f', 48}, {'g', 49}, {'h', 50}, 
								      {'j', 51}, {'k', 52}, {'l', 53}, {';', 54}, {'\'', 55},
								  {'q', 50}, {'w', 51}, {'e', 52}, {'r', 53}, {'t', 54}, {'y', 55}, 
								      {'u', 56}, {'i', 57}, {'o', 58}, {'p', 59}, {'[', 60}, {']', 61}, {'\\', 62},
								  {'1', 55}, {'2', 56}, {'3', 57}, {'4', 58}, {'5', 59}, {'6', 60}, 
								      {'7', 61}, {'8', 62}, {'9', 63}, {'0', 64}, {'-', 65}, {'=', 66}};

const std::string alphabet = "zxcvbnm,./asdfghjkl;\'qwertyuiop[]\\1234567890-=";
const int keys = 46;
bool focus = true;

int transp = -12;

std::map<unsigned char, int> eventmap;
std::map<unsigned char, int> presscounts;
std::map<unsigned char, int> oldcounts;
std::map<unsigned char, int> pressdeltas;

void dict_init()
{
	for (int i = 0; i < keys; i++)
	{
		presscounts[alphabet[i]] = 0;
		oldcounts[alphabet[i]] = 0;
		pressdeltas[alphabet[i]] = 0;
	}

	eventmap['a'] = sf::Keyboard::A;
	eventmap['b'] = sf::Keyboard::B;
	eventmap['c'] = sf::Keyboard::C;
	eventmap['d'] = sf::Keyboard::D;
	eventmap['e'] = sf::Keyboard::E;
	eventmap['f'] = sf::Keyboard::F;
	eventmap['g'] = sf::Keyboard::G;
	eventmap['h'] = sf::Keyboard::H;
	eventmap['i'] = sf::Keyboard::I;
	eventmap['j'] = sf::Keyboard::J;
	eventmap['k'] = sf::Keyboard::K;
	eventmap['l'] = sf::Keyboard::L;
	eventmap['m'] = sf::Keyboard::M;
	eventmap['n'] = sf::Keyboard::N;
	eventmap['o'] = sf::Keyboard::O;
	eventmap['p'] = sf::Keyboard::P;
	eventmap['q'] = sf::Keyboard::Q;
	eventmap['r'] = sf::Keyboard::R;
	eventmap['s'] = sf::Keyboard::S;
	eventmap['t'] = sf::Keyboard::T;
	eventmap['u'] = sf::Keyboard::U;
	eventmap['v'] = sf::Keyboard::V;
	eventmap['w'] = sf::Keyboard::W;
	eventmap['x'] = sf::Keyboard::X;
	eventmap['y'] = sf::Keyboard::Y;
	eventmap['z'] = sf::Keyboard::Z;

	eventmap[','] = sf::Keyboard::Comma;
	eventmap['.'] = sf::Keyboard::Period;
	eventmap['/'] = sf::Keyboard::Slash;
	eventmap[';'] = sf::Keyboard::Semicolon;
	eventmap['\''] = sf::Keyboard::Quote;
	eventmap['['] = sf::Keyboard::LBracket;
	eventmap[']'] = sf::Keyboard::RBracket;
	eventmap['\\'] = sf::Keyboard::Backslash;

	eventmap['1'] = sf::Keyboard::Num1;
	eventmap['2'] = sf::Keyboard::Num2;
	eventmap['3'] = sf::Keyboard::Num3;
	eventmap['4'] = sf::Keyboard::Num4;
	eventmap['5'] = sf::Keyboard::Num5;
	eventmap['6'] = sf::Keyboard::Num6;
	eventmap['7'] = sf::Keyboard::Num7;
	eventmap['8'] = sf::Keyboard::Num8;
	eventmap['9'] = sf::Keyboard::Num9;
	eventmap['0'] = sf::Keyboard::Num0;
	eventmap['-'] = sf::Keyboard::Hyphen;
	eventmap['='] = sf::Keyboard::Equal;
}


MidiOut MO = MidiOut();

int main()
{
	dict_init();

	MO.startup();
	MO.getports(false);
	MO.open("IAC Driver Bus 1");

	// sf::ContextSettings settings;
	// settings.antialiasingLevel = 1;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT),
		"Typing",
		sf::Style::Titlebar | sf::Style::Close);

	window.setFramerateLimit(120);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
					window.close();

				if (event.key.code == sf::Keyboard::RShift)
				{
					transp += 5;
					transp = std::min(8, transp);
				}

				if (event.key.code == sf::Keyboard::LShift)
				{
					transp -= 5;
					transp = std::max(-12, transp);
				}
			}

			else if (event.type == sf::Event::GainedFocus) focus = true;
			else if (event.type == sf::Event::LostFocus) focus = false;

		}

		// if (focus)
		if (true)
		{
			for (int i = 0; i < keys; i++)
			{
				presscounts[alphabet[i]] = 0;
			}

			for (int i = 0; i < keys; i++)
			{
				if (sf::Keyboard::isKeyPressed((sf::Keyboard::Key)eventmap[alphabet[i]]))
				{
					presscounts[alphabet[i]] = 1;
				}
			}

			for (int i = 0; i < keys; i++)
			{
				pressdeltas[alphabet[i]] = presscounts[alphabet[i]] - oldcounts[alphabet[i]];
				oldcounts[alphabet[i]] = presscounts[alphabet[i]];
			}

			for (int i = 0; i < keys; i++)
			{
				if (pressdeltas[alphabet[i]] == 1)
				{
					// std::cout << alphabet[i] << " pressed." << std::endl;
					std::vector<unsigned char> message({{144, (unsigned char)(keymapping[alphabet[i]] + transp), 127}});
					MO.send(&message);
				}
				else if (pressdeltas[alphabet[i]] == -1)
				{
					// std::cout << alphabet[i] << " released." << std::endl;
					std::vector<unsigned char> message({{144, (unsigned char)(keymapping[alphabet[i]] + transp), 0}});
					MO.send(&message);
				}
			}

		}

		// sf::sleep(sf::microseconds(100));

		// update
		// render
		// window.clear(); // clear old game

		// window.display(); // window is done drawing
	}

	return 0;
}