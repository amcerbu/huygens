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
#define MARGIN 0.25

#define WIDTH (13 + MARGIN) * RADIUS
#define HEIGHT (5 + MARGIN) * RADIUS
#define KEYS 58


using namespace soundmath;

bool focus = true;
bool focusless = false; // runs without focus

int bottom = 28;
int shifts = 7;
int tuning = 5;
int origin = bottom; // low E on the bass
int span = 4 * tuning + 12 + shifts * tuning;



std::map<int, int> transpositions; // transpositions from base: of form sf::Keyboard -> int

std::map<int, std::string> glyphs; // transpositions from base: of form sf::Keyboard -> int

std::map<int, std::set<int>> responsible; // key: midi note, value: set of keyboard codes holding
std::map<int, int> active; // key: midi note, value: number of buttons responsible (positive iff note is active)
std::map<int, int> deltas; // key: midi note, value: checks whether active goes from 0 to positive or positive to 0


void dict_init()
{
	transpositions[sf::Keyboard::Z] = 0 * tuning + 0;			glyphs[sf::Keyboard::Z] = "Z";
	transpositions[sf::Keyboard::X] = 0 * tuning + 1;			glyphs[sf::Keyboard::X] = "X";
	transpositions[sf::Keyboard::C] = 0 * tuning + 2;			glyphs[sf::Keyboard::C] = "C";
	transpositions[sf::Keyboard::V] = 0 * tuning + 3;			glyphs[sf::Keyboard::V] = "V";
	transpositions[sf::Keyboard::B] = 0 * tuning + 4;			glyphs[sf::Keyboard::B] = "B";
	transpositions[sf::Keyboard::N] = 0 * tuning + 5;			glyphs[sf::Keyboard::N] = "N";
	transpositions[sf::Keyboard::M] = 0 * tuning + 6;			glyphs[sf::Keyboard::M] = "M";
	transpositions[sf::Keyboard::Comma] = 0 * tuning + 7;		glyphs[sf::Keyboard::Comma] = ",";
	transpositions[sf::Keyboard::Period] = 0 * tuning + 8;		glyphs[sf::Keyboard::Period] = ".";
	transpositions[sf::Keyboard::Slash] = 0 * tuning + 9;		glyphs[sf::Keyboard::Slash] = "/";

	transpositions[sf::Keyboard::A] = 1 * tuning + 0;			glyphs[sf::Keyboard::A] = "A";
	transpositions[sf::Keyboard::S] = 1 * tuning + 1;			glyphs[sf::Keyboard::S] = "S";
	transpositions[sf::Keyboard::D] = 1 * tuning + 2;			glyphs[sf::Keyboard::D] = "D";
	transpositions[sf::Keyboard::F] = 1 * tuning + 3;			glyphs[sf::Keyboard::F] = "F";
	transpositions[sf::Keyboard::G] = 1 * tuning + 4;			glyphs[sf::Keyboard::G] = "G";
	transpositions[sf::Keyboard::H] = 1 * tuning + 5;			glyphs[sf::Keyboard::H] = "H";
	transpositions[sf::Keyboard::J] = 1 * tuning + 6;			glyphs[sf::Keyboard::J] = "J";
	transpositions[sf::Keyboard::K] = 1 * tuning + 7;			glyphs[sf::Keyboard::K] = "K";
	transpositions[sf::Keyboard::L] = 1 * tuning + 8;			glyphs[sf::Keyboard::L] = "L";
	transpositions[sf::Keyboard::Semicolon] = 1 * tuning + 9;	glyphs[sf::Keyboard::Semicolon] = ";";
	transpositions[sf::Keyboard::Quote] = 1 * tuning + 10;		glyphs[sf::Keyboard::Quote] = "'";

	transpositions[sf::Keyboard::Q] = 2 * tuning + 0;			glyphs[sf::Keyboard::Q] = "Q";
	transpositions[sf::Keyboard::W] = 2 * tuning + 1;			glyphs[sf::Keyboard::W] = "W";
	transpositions[sf::Keyboard::E] = 2 * tuning + 2;			glyphs[sf::Keyboard::E] = "E";
	transpositions[sf::Keyboard::R] = 2 * tuning + 3;			glyphs[sf::Keyboard::R] = "R";
	transpositions[sf::Keyboard::T] = 2 * tuning + 4;			glyphs[sf::Keyboard::T] = "T";
	transpositions[sf::Keyboard::Y] = 2 * tuning + 5;			glyphs[sf::Keyboard::Y] = "Y";
	transpositions[sf::Keyboard::U] = 2 * tuning + 6;			glyphs[sf::Keyboard::U] = "U";
	transpositions[sf::Keyboard::I] = 2 * tuning + 7;			glyphs[sf::Keyboard::I] = "I";
	transpositions[sf::Keyboard::O] = 2 * tuning + 8;			glyphs[sf::Keyboard::O] = "O";
	transpositions[sf::Keyboard::P] = 2 * tuning + 9;			glyphs[sf::Keyboard::P] = "P";
	transpositions[sf::Keyboard::LBracket] = 2 * tuning + 10;	glyphs[sf::Keyboard::LBracket] = "[";
	transpositions[sf::Keyboard::RBracket] = 2 * tuning + 11;	glyphs[sf::Keyboard::RBracket] = "]";
	transpositions[sf::Keyboard::Backslash] = 2 * tuning + 12;	glyphs[sf::Keyboard::Backslash] = "\\";

	transpositions[sf::Keyboard::Num1] = 3 * tuning + 0;		glyphs[sf::Keyboard::Num1] = "1";
	transpositions[sf::Keyboard::Num2] = 3 * tuning + 1;		glyphs[sf::Keyboard::Num2] = "2";
	transpositions[sf::Keyboard::Num3] = 3 * tuning + 2;		glyphs[sf::Keyboard::Num3] = "3";
	transpositions[sf::Keyboard::Num4] = 3 * tuning + 3;		glyphs[sf::Keyboard::Num4] = "4";
	transpositions[sf::Keyboard::Num5] = 3 * tuning + 4;		glyphs[sf::Keyboard::Num5] = "5";
	transpositions[sf::Keyboard::Num6] = 3 * tuning + 5;		glyphs[sf::Keyboard::Num6] = "6";
	transpositions[sf::Keyboard::Num7] = 3 * tuning + 6;		glyphs[sf::Keyboard::Num7] = "7";
	transpositions[sf::Keyboard::Num8] = 3 * tuning + 7;		glyphs[sf::Keyboard::Num8] = "8";
	transpositions[sf::Keyboard::Num9] = 3 * tuning + 8;		glyphs[sf::Keyboard::Num9] = "9";
	transpositions[sf::Keyboard::Num0] = 3 * tuning + 9;		glyphs[sf::Keyboard::Num0] = "0";
	transpositions[sf::Keyboard::Hyphen] = 3 * tuning + 10;		glyphs[sf::Keyboard::Hyphen] = "-";
	transpositions[sf::Keyboard::Equal] = 3 * tuning + 11;		glyphs[sf::Keyboard::Equal] = "=";

	transpositions[sf::Keyboard::F1] = 4 * tuning + 0;			glyphs[sf::Keyboard::F1] = "F1";
	transpositions[sf::Keyboard::F2] = 4 * tuning + 1;			glyphs[sf::Keyboard::F2] = "F2";
	transpositions[sf::Keyboard::F3] = 4 * tuning + 2;			glyphs[sf::Keyboard::F3] = "F3";
	transpositions[sf::Keyboard::F4] = 4 * tuning + 3;			glyphs[sf::Keyboard::F4] = "F4";
	transpositions[sf::Keyboard::F5] = 4 * tuning + 4;			glyphs[sf::Keyboard::F5] = "F5";
	transpositions[sf::Keyboard::F6] = 4 * tuning + 5;			glyphs[sf::Keyboard::F6] = "F6";
	transpositions[sf::Keyboard::F7] = 4 * tuning + 6;			glyphs[sf::Keyboard::F7] = "F7";
	transpositions[sf::Keyboard::F8] = 4 * tuning + 7;			glyphs[sf::Keyboard::F8] = "F8";
	transpositions[sf::Keyboard::F9] = 4 * tuning + 8;			glyphs[sf::Keyboard::F9] = "F9";
	transpositions[sf::Keyboard::F10] = 4 * tuning + 9;			glyphs[sf::Keyboard::F10] = "F0";
	transpositions[sf::Keyboard::F11] = 4 * tuning + 10;		glyphs[sf::Keyboard::F11] = "F1";
	transpositions[sf::Keyboard::F12] = 4 * tuning + 11;		glyphs[sf::Keyboard::F12] = "F2";

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

	squares[sf::Keyboard::Z].setPosition((0 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::X].setPosition((1 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::C].setPosition((2 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::V].setPosition((3 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::B].setPosition((4 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::N].setPosition((5 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::M].setPosition((6 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Comma].setPosition((7 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Period].setPosition((8 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Slash].setPosition((9 + MARGIN) * RADIUS, (4 + MARGIN) * RADIUS);

	squares[sf::Keyboard::A].setPosition((0 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::S].setPosition((1 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::D].setPosition((2 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F].setPosition((3 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::G].setPosition((4 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::H].setPosition((5 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::J].setPosition((6 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::K].setPosition((7 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::L].setPosition((8 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Semicolon].setPosition((9 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Quote].setPosition((10 + MARGIN) * RADIUS, (3 + MARGIN) * RADIUS);

	squares[sf::Keyboard::Q].setPosition((0 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::W].setPosition((1 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::E].setPosition((2 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::R].setPosition((3 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::T].setPosition((4 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Y].setPosition((5 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::U].setPosition((6 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::I].setPosition((7 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::O].setPosition((8 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::P].setPosition((9 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::LBracket].setPosition((10 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::RBracket].setPosition((11 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Backslash].setPosition((12 + MARGIN) * RADIUS, (2 + MARGIN) * RADIUS);

	squares[sf::Keyboard::Num1].setPosition((0 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num2].setPosition((1 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num3].setPosition((2 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num4].setPosition((3 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num5].setPosition((4 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num6].setPosition((5 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num7].setPosition((6 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num8].setPosition((7 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num9].setPosition((8 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Num0].setPosition((9 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Hyphen].setPosition((10 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);
	squares[sf::Keyboard::Equal].setPosition((11 + MARGIN) * RADIUS, (1 + MARGIN) * RADIUS);

	squares[sf::Keyboard::F1].setPosition((0 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F2].setPosition((1 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F3].setPosition((2 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F4].setPosition((3 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F5].setPosition((4 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F6].setPosition((5 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F7].setPosition((6 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F8].setPosition((7 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F9].setPosition((8 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F10].setPosition((9 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F11].setPosition((10 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
	squares[sf::Keyboard::F12].setPosition((11 + MARGIN) * RADIUS, (0 + MARGIN) * RADIUS);
}

MidiOut MO = MidiOut();

int main()
{
	dict_init();
	shape_init();

	MO.startup();
	MO.getports(false);
	MO.open("IAC Driver Bus 1");

	sf::Font menlo;
	if (!menlo.loadFromFile("Menlo.ttf"))
	{
		std::cout << "Error loading font." << std::endl;
		return 0;
	}

	sf::Text text;
	text.setFont(menlo);
	text.setFillColor(sf::Color::White);

	sf::ContextSettings settings;
	settings.antialiasingLevel = 1;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT),
		"Typing",
		sf::Style::Titlebar | sf::Style::Close, settings);


	window.setFramerateLimit(30);

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

				else if (event.key.code == sf::Keyboard::Tab)
				{
					origin += tuning;
					origin = std::min(bottom + shifts * tuning, origin);
				}

				else if (event.key.code == sf::Keyboard::LShift)
				{
					origin -= tuning;
					origin = std::max(bottom, origin);
				}
			}

			else if (event.type == sf::Event::GainedFocus) focus = true;
			else if (event.type == sf::Event::LostFocus) focus = false;
		}

		if (focusless || focus)
		{
			for (const auto &pair : transpositions)
			{
				if (sf::Keyboard::isKeyPressed((sf::Keyboard::Key)pair.first))
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
				if (deltas[i] == 1)
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
	}

	return 0;
}