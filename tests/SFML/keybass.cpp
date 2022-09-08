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

#define RADIUS 100
#define MARGIN 0.1
#define kb sf::Keyboard
#define GUI true // turn gui on?

#define WIDTH (13 + MARGIN) * RADIUS
#define HEIGHT (5 + MARGIN) * RADIUS

using namespace soundmath;

bool focus = true;
bool focusless = true; // runs without focus

int bottom = 21; // A0
int shifts = 12;
int tuning = 5;
int origin = 36; // C on A string of bass
int span = 4 * tuning + 12 + shifts * tuning;



std::map<int, int> transpositions; // transpositions from base: of form kb -> int

std::map<int, std::string> glyphs; // transpositions from base: of form kb -> int

std::map<int, std::set<int>> responsible; // key: midi note, value: set of keyboard codes holding
std::map<int, int> active; // key: midi note, value: number of buttons responsible (positive iff note is active)
std::map<int, int> deltas; // key: midi note, value: checks whether active goes from 0 to positive or positive to 0

void dict_init()
{
	transpositions[kb::Z] = 0 * tuning + 0;				glyphs[kb::Z] = "Z";
	transpositions[kb::X] = 0 * tuning + 1;				glyphs[kb::X] = "X";
	transpositions[kb::C] = 0 * tuning + 2;				glyphs[kb::C] = "C";
	transpositions[kb::V] = 0 * tuning + 3;				glyphs[kb::V] = "V";
	transpositions[kb::B] = 0 * tuning + 4;				glyphs[kb::B] = "B";
	transpositions[kb::N] = 0 * tuning + 5;				glyphs[kb::N] = "N";
	transpositions[kb::M] = 0 * tuning + 6;				glyphs[kb::M] = "M";
	transpositions[kb::Comma] = 0 * tuning + 7;			glyphs[kb::Comma] = ",";
	transpositions[kb::Period] = 0 * tuning + 8;		glyphs[kb::Period] = ".";
	transpositions[kb::Slash] = 0 * tuning + 9;			glyphs[kb::Slash] = "/";
	transpositions[kb::RShift] = 0 * tuning + 10;		glyphs[kb::RShift] = "Sh";

	transpositions[kb::A] = 1 * tuning + 0;				glyphs[kb::A] = "A";
	transpositions[kb::S] = 1 * tuning + 1;				glyphs[kb::S] = "S";
	transpositions[kb::D] = 1 * tuning + 2;				glyphs[kb::D] = "D";
	transpositions[kb::F] = 1 * tuning + 3;				glyphs[kb::F] = "F";
	transpositions[kb::G] = 1 * tuning + 4;				glyphs[kb::G] = "G";
	transpositions[kb::H] = 1 * tuning + 5;				glyphs[kb::H] = "H";
	transpositions[kb::J] = 1 * tuning + 6;				glyphs[kb::J] = "J";
	transpositions[kb::K] = 1 * tuning + 7;				glyphs[kb::K] = "K";
	transpositions[kb::L] = 1 * tuning + 8;				glyphs[kb::L] = "L";
	transpositions[kb::Semicolon] = 1 * tuning + 9;		glyphs[kb::Semicolon] = ";";
	transpositions[kb::Quote] = 1 * tuning + 10;		glyphs[kb::Quote] = "'";
	transpositions[kb::Return] = 1 * tuning + 11;		glyphs[kb::Return] = "Re";

	transpositions[kb::Q] = 2 * tuning + 0;				glyphs[kb::Q] = "Q";
	transpositions[kb::W] = 2 * tuning + 1;				glyphs[kb::W] = "W";
	transpositions[kb::E] = 2 * tuning + 2;				glyphs[kb::E] = "E";
	transpositions[kb::R] = 2 * tuning + 3;				glyphs[kb::R] = "R";
	transpositions[kb::T] = 2 * tuning + 4;				glyphs[kb::T] = "T";
	transpositions[kb::Y] = 2 * tuning + 5;				glyphs[kb::Y] = "Y";
	transpositions[kb::U] = 2 * tuning + 6;				glyphs[kb::U] = "U";
	transpositions[kb::I] = 2 * tuning + 7;				glyphs[kb::I] = "I";
	transpositions[kb::O] = 2 * tuning + 8;				glyphs[kb::O] = "O";
	transpositions[kb::P] = 2 * tuning + 9;				glyphs[kb::P] = "P";
	transpositions[kb::LBracket] = 2 * tuning + 10;		glyphs[kb::LBracket] = "[";
	transpositions[kb::RBracket] = 2 * tuning + 11;		glyphs[kb::RBracket] = "]";
	transpositions[kb::Backslash] = 2 * tuning + 12;	glyphs[kb::Backslash] = "\\";

	transpositions[kb::Num1] = 3 * tuning + 0;			glyphs[kb::Num1] = "1";
	transpositions[kb::Num2] = 3 * tuning + 1;			glyphs[kb::Num2] = "2";
	transpositions[kb::Num3] = 3 * tuning + 2;			glyphs[kb::Num3] = "3";
	transpositions[kb::Num4] = 3 * tuning + 3;			glyphs[kb::Num4] = "4";
	transpositions[kb::Num5] = 3 * tuning + 4;			glyphs[kb::Num5] = "5";
	transpositions[kb::Num6] = 3 * tuning + 5;			glyphs[kb::Num6] = "6";
	transpositions[kb::Num7] = 3 * tuning + 6;			glyphs[kb::Num7] = "7";
	transpositions[kb::Num8] = 3 * tuning + 7;			glyphs[kb::Num8] = "8";
	transpositions[kb::Num9] = 3 * tuning + 8;			glyphs[kb::Num9] = "9";
	transpositions[kb::Num0] = 3 * tuning + 9;			glyphs[kb::Num0] = "0";
	transpositions[kb::Hyphen] = 3 * tuning + 10;		glyphs[kb::Hyphen] = "-";
	transpositions[kb::Equal] = 3 * tuning + 11;		glyphs[kb::Equal] = "=";
	transpositions[kb::Backspace] = 3 * tuning + 12;	glyphs[kb::Backspace] = "De";

	transpositions[kb::F1] = 4 * tuning + 0;			glyphs[kb::F1] = "F1";
	transpositions[kb::F2] = 4 * tuning + 1;			glyphs[kb::F2] = "F2";
	transpositions[kb::F3] = 4 * tuning + 2;			glyphs[kb::F3] = "F3";
	transpositions[kb::F4] = 4 * tuning + 3;			glyphs[kb::F4] = "F4";
	transpositions[kb::F5] = 4 * tuning + 4;			glyphs[kb::F5] = "F5";
	transpositions[kb::F6] = 4 * tuning + 5;			glyphs[kb::F6] = "F6";
	transpositions[kb::F7] = 4 * tuning + 6;			glyphs[kb::F7] = "F7";
	transpositions[kb::F8] = 4 * tuning + 7;			glyphs[kb::F8] = "F8";
	transpositions[kb::F9] = 4 * tuning + 8;			glyphs[kb::F9] = "F9";
	transpositions[kb::F10] = 4 * tuning + 9;			glyphs[kb::F10] = "F0";
	transpositions[kb::F11] = 4 * tuning + 10;			glyphs[kb::F11] = "F1";
	transpositions[kb::F12] = 4 * tuning + 11;			glyphs[kb::F12] = "F2";

	for (int i = bottom; i < bottom + span; i++)
	{
		responsible[i] = std::set<int>();
		active[i] = 0;
		deltas[i] = 0;
	}
}

std::map<int, sf::RectangleShape> squares;

void colorize()
{
	for (const auto &pair : transpositions)
	{
		switch ((pair.second + origin) % 12)
		{
			case 0:
				squares[pair.first].setFillColor(
					active[pair.second + origin] ? sf::Color(192, 16, 0) : sf::Color(192, 96, 0));
				break;

			case 1:
			case 3:
			case 6:
			case 8:
			case 10:
				squares[pair.first].setFillColor(
					active[pair.second + origin] ? sf::Color(32, 32, 192) : sf::Color(32, 32, 32));
				break;

			default:
				squares[pair.first].setFillColor(
					active[pair.second + origin] ? sf::Color(96, 96, 255) : sf::Color(127, 127, 127));
				break;
		}
	}
}

void shape_init()
{
	for (const auto &pair : transpositions)
	{
		squares[pair.first] = sf::RectangleShape(sf::Vector2f(RADIUS * (1 - MARGIN), RADIUS * (1 - MARGIN)));
	}

	squares[kb::Z].setPosition((0 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::X].setPosition((1 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::C].setPosition((2 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::V].setPosition((3 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::B].setPosition((4 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::N].setPosition((5 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::M].setPosition((6 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::Comma].setPosition((7 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::Period].setPosition((8 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::Slash].setPosition((9 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[kb::RShift].setPosition((10 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);

	squares[kb::A].setPosition((0 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::S].setPosition((1 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::D].setPosition((2 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::F].setPosition((3 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::G].setPosition((4 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::H].setPosition((5 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::J].setPosition((6 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::K].setPosition((7 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::L].setPosition((8 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::Semicolon].setPosition((9 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::Quote].setPosition((10 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[kb::Return].setPosition((11 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);

	squares[kb::Q].setPosition((0 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::W].setPosition((1 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::E].setPosition((2 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::R].setPosition((3 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::T].setPosition((4 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::Y].setPosition((5 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::U].setPosition((6 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::I].setPosition((7 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::O].setPosition((8 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::P].setPosition((9 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::LBracket].setPosition((10 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::RBracket].setPosition((11 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[kb::Backslash].setPosition((12 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);

	squares[kb::Num1].setPosition((0 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num2].setPosition((1 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num3].setPosition((2 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num4].setPosition((3 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num5].setPosition((4 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num6].setPosition((5 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num7].setPosition((6 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num8].setPosition((7 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num9].setPosition((8 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Num0].setPosition((9 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Hyphen].setPosition((10 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Equal].setPosition((11 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[kb::Backspace].setPosition((12 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);

	squares[kb::F1].setPosition((0 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F2].setPosition((1 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F3].setPosition((2 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F4].setPosition((3 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F5].setPosition((4 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F6].setPosition((5 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F7].setPosition((6 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F8].setPosition((7 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F9].setPosition((8 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F10].setPosition((9 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F11].setPosition((10 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[kb::F12].setPosition((11 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
}

MidiOut MO = MidiOut();

int main()
{
	const double framerate = 30;
	sf::Time frametime = sf::microseconds(1000000 / framerate);
	sf::Clock clock;
	sf::Text text;
	sf::Font menlo;

	dict_init();
	if (GUI)
	{
		shape_init();
		if (!menlo.loadFromFile("Font.ttf"))
		{
			std::cout << "Error loading font." << std::endl;
			return 0;
		}

		text.setFont(menlo);
		text.setFillColor(sf::Color::White);
	}

	MO.startup();
	MO.getports(false);
	MO.open("IAC Driver Bus 1");

	sf::ContextSettings settings;
	settings.antialiasingLevel = 1;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT),
		"Typing",
		sf::Style::Titlebar | sf::Style::Close, settings);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			else if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == kb::Escape)
					window.close();

				else if (event.key.code == kb::Up)
				{
					if (origin + tuning <= bottom + shifts * tuning)
						origin += tuning;
				}

				else if (event.key.code == kb::Down)
				{
					if (origin - tuning >= bottom)
						origin -= tuning;
				}

				else if (event.key.code == kb::Right)
				{
					if (origin + 1 <= bottom + shifts * tuning)
						origin += 1;
				}

				else if (event.key.code == kb::Left)
				{
					if (origin - 1 >= bottom)
						origin -= 1;
				}
			}

			else if (event.type == sf::Event::GainedFocus) focus = true;
			else if (event.type == sf::Event::LostFocus) focus = false;
		}

		if (focusless || focus)
		{
			for (const auto &pair : transpositions)
			{
				if (kb::isKeyPressed((kb::Key)pair.first))
				{
					responsible[pair.second + origin].insert(pair.first);
				}
				else
				{
					for (int i = bottom; i < bottom + span; i++)
					{
						responsible[i].erase(pair.first);
					}
				}
			}

			for (int i = bottom; i < bottom + span; i++)
			{
				int size = responsible[i].size();

				if (size == 0 && active[i] > 0)
					deltas[i] = -1;
				else if (active[i] < size)
					deltas[i] = 1;
				else
					deltas[i] = 0;

				active[i] = size;
			}

			for (int i = bottom; i < bottom + span; i++)
			{
				if (deltas[i] == 1 && kb::isKeyPressed(kb::Space))
				{
					// std::cout << i << " pressed." << std::endl;
					std::vector<unsigned char> message({{144, (unsigned char)(i), 127}});
					MO.send(&message);
				}
				else if (deltas[i] == -1)
				{
					// std::cout << i << " released." << std::endl;
					std::vector<unsigned char> message({{144, (unsigned char)(i), 0}});
					MO.send(&message);	
				}
			}
		}

		if (GUI && clock.getElapsedTime() > frametime)
		{
			window.clear(sf::Color(192, 192, 192));
			colorize();
			for (const auto &pair : transpositions)
			{
				window.draw(squares[pair.first]);
				text.setString(glyphs[pair.first]);
				text.setPosition(squares[pair.first].getPosition());
				window.draw(text);
			}

			window.display();
			clock.restart();
		}

		sf::sleep(sf::microseconds(100));
	}

	return 0;
}


