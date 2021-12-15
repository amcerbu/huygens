// polyres.h
#include "includes.h"
#include "filter.h"
#include "particle.h"

#ifndef POLYRES

namespace soundmath
{
	template <typename T> class Polyres
	{
	public:
		uint voices; // number of harmonic series
		uint overtones; // number of overtones per series
		T decay; // rate of decay of overtones
		T normalization; // to accommodate different decay rates
		T harmonicity; // harmonicity

		Filter<T>* filters;
		Particle* particles;
		Particle* guides;
		Spring* springs;
		Gravity* gravities;

		T* active;
		T* amplitudes;
		T* resonances;
		T* pitches;
		
		T Q;

		~Polyres()
		{
			delete [] filters;
			delete [] particles;
			delete [] guides;
			delete [] springs;
			delete [] gravities;

			delete [] active;
			delete [] amplitudes;
			delete [] resonances;
			delete [] pitches;
		}

		Polyres(uint voices, uint overtones, T decay, T Q = 0.99999, T harmonicity = 1)
		{
			this->voices = voices;
			this->overtones = overtones;
			this->decay = decay;
			this->normalization = decay != 1 ? (1 - pow(decay, overtones)) / (1 - decay) : overtones;
			this->harmonicity = harmonicity;
			this->Q = Q;

			filters = new Filter<T>[voices * overtones];
			particles = new Particle[voices * overtones];
			guides = new Particle[voices];
			springs = new Spring[voices * overtones];
			gravities = new Gravity[voices * (voices - 1) * overtones * overtones / 2];

			active = new T[voices];
			amplitudes = new T[voices];
			resonances = new T[voices];
			pitches = new T[voices];

			for (int i = 0; i < voices; i++)
				for (int j = 0; j < overtones; j++)
				{
					filters[i * overtones + j].initialize(3);
				}

			memset(active, 0, voices * sizeof(T));
			memset(amplitudes, 0, voices * sizeof(T));
			memset(resonances, 0, voices * sizeof(T));

			for (int i = 0; i < voices; i++)
			{
				springs[i * overtones].bind(&guides[i], &particles[i * overtones]);
				springs[i * overtones].strength(2 * FORCE);
				for (int j = 1; j < overtones; j++)
				{
					springs[i * overtones + j].bind(&particles[i * overtones + j - 1], &particles[i * overtones + j]);
					springs[i * overtones + j].strength(1 * FORCE);
				}

			}

			int count = 0;
			for (int i = 0; i < voices; i++)
				for (int j = i + 1; j < voices; j++)
					for (int k = 0; k < overtones; k++)
						for (int m = 0; m < overtones; m++)
						{
							gravities[count].bind(&particles[i * overtones + k], &particles[j * overtones + m]);
							gravities[count].strength(1 * FORCE);
							count++;
						}
		}

		T operator()(T input)
		{
			T sample = 0;
			for (int i = 0; i < voices; i++)
				if (amplitudes[i])
					for (int j = 0; j < overtones; j++)
						sample += amplitudes[i] * pow(decay, j) * filters[i * overtones + j](input) / (2 * voices * normalization);

			return (T)sample;
		}

		void physics()
		{
			// zero the forces on particles
			for (int i = 0; i < voices; i++)
				if (active[i])
				{
					guides[i].prepare();
					guides[i].mass = active[i];
					for (int j = 0; j < overtones; j++)
					{
						particles[i * overtones + j].prepare();
						particles[i * overtones + j].mass = active[i];
					}
				}

			// apply spring forces
			for (int i = 0; i < voices; i++)
				if (active[i])
					for (int j = 0; j < overtones; j++)
						springs[i * overtones + j].tick();

			// apply forces from gravity
			int count = 0;
			for (int i = 0; i < voices; i++)
			{
				if (active[i]) // if one voice is active
					for (int j = i + 1; j < voices; j++)
					{
						if (active[j]) // as is another
							for (int k = 0; k < overtones; k++)
								for (int m = 0; m < overtones; m++)
								{
									gravities[count].tick(); // allow their overtones to interact
									count++;
								}
						else
							count += overtones * overtones;
					}
				else
					count += overtones * overtones * (voices - i - 1);
			}

			// tick the particles
			for (int i = 0; i < voices; i++)
				if (active[i])
					for (int j = 0; j < overtones; j++)
						particles[i * overtones + j].tick();
		}

		void tick()
		{
			for (int i = 0; i < voices; i++)
				amplitudes[i] = 0.005 * active[i] + 0.995 * amplitudes[i];

			// update oscillator frequencies; tick oscillators
			for (int i = 0; i < voices; i++)
				if (active[i] || amplitudes[i])
					for (int j = 0; j < overtones; j++)
					{
						filters[i * overtones + j].resonant(mtof(particles[i * overtones + j]()), resonances[i]);
						filters[i * overtones + j].tick();
					}
		}

		// request new voice at a given frequency; return voice number
		int request(T fundamental, T amplitude = 0)
		{
			int voice = -1;
			for (int i = 0; i < voices; i++)
			{
				if (!active[i])
				{
					voice = i;
					break;
				}
			}

			if (voice >= 0)
			{
				resonances[voice] = Q;
				T frequency = fundamental;
				T previous;
				active[voice] = amplitude;
				guides[voice].initialize(1, ftom(fundamental));
				for (int j = 0; j < overtones; j++)
				{
					previous = frequency;
					frequency = fundamental * pow(j + 1, harmonicity);

					particles[voice * overtones + j].initialize(1, ftom(frequency));
					// filters[voice * overtones + j].forget();
					springs[voice * overtones + j].target(ftom(frequency) - ftom(previous));
				}
			}

			return voice;
		}

		void release(int voice)
		{
			if (voice >= 0)
			{
				active[voice] = 0;
				return;
			}

			memset(active, 0, voices * sizeof(T));
		}

		void makenote(T pitch, T amplitude)
		{
			int voice = request(mtof(pitch), amplitude);
			if (voice >= 0)
				pitches[voice] = pitch;
		}

		void aftertouch(T pitch, T amplitude)
		{
			int voice = -1;
			for (int i = 0; i < voices; i++)
			{
				if (pitches[i] == pitch)
				{
					voice = i;
					break;
				}
			}

			if (voice >= 0)
			{
				resonances[voice] = amplitude;
			}
		}


		void endnote(T pitch)
		{
			for (int j = 0; j < voices; j++)
				if (pitches[j] == pitch)
					release(j);
		}
	};
}

#define POLYRES
#endif