// subtractive.h
#include "includes.h"
#include "minimizer.h"
#include "filterbank.h"
#include "filter.h"

#ifndef SUBTRACTIVE

namespace soundmath
{
	template <typename T> class Subtractive : public Minimizer<T>
	{
	public:
		using Minimizer<T>::voices, Minimizer<T>::overtones, Minimizer<T>::decay, Minimizer<T>::harmonicity,
			  Minimizer<T>::particles, Minimizer<T>::active, Minimizer<T>::pitches;

		Subtractive() { }
		~Subtractive()
		{
			delete [] amplitudes;

			for (int i = 0; i < voices * overtones; i++)
				delete filters[i];
			delete [] filters;
		}

		Subtractive(uint voices, uint overtones, T decay, T harmonicity = 1.0, T k = 0.1) : 
			Minimizer<T>(voices, overtones, decay, harmonicity), attack(relaxation(k))
		{
			normalization = decay != 1 ? (1 - pow(decay, overtones)) / (1 - decay) : overtones;
			amplitudes = new T[voices];
			memset(amplitudes, 0, voices * sizeof(T));

			filters = new Filter<T>*[voices * overtones];
			for (int i = 0; i < voices * overtones; i++)
				filters[i] = new Filter<T>({1,0,0},{0});
		}

		void tick()
		{
			for (int i = 0; i < voices; i++)
				amplitudes[i] = (1 - attack) * active[i] + attack * amplitudes[i];

			// update and tick filters
			for (int i = 0; i < voices; i++)
				if (amplitudes[i])
					for (int j = 0; j < overtones; j++)
					{
						T frequency = mtof(particles[i * overtones + j]());
						filters[i * overtones + j]->resonant(frequency, 0.99999);
						filters[i * overtones + j]->tick();
					}
		}

		T operator()(T sample)
		{
			T out = 0;
			for (int i = 0; i < voices; i++)
				if (amplitudes[i])
					for (int j = 0; j < overtones; j++)
						out += amplitudes[i] * pow(decay, j) * (*filters[i * overtones + j])(sample) / (voices * normalization);

			return out;
		}

	private:
		Filter<T>** filters;
		T* amplitudes;
		T normalization;
		T attack;
	};
}

#define SUBTRACTIVE
#endif