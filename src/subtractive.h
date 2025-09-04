// subtractive.h
#include "includes.h"
#include "minimizer.h"
#include "filterbank.h"
#include "filter.h"
#include "filterbank.h"

#ifndef SUBTRACTIVE

#define MIMO

namespace soundmath
{

#ifdef MIMO
	// N-channel version (for use with multichannel noise signals)
		template <typename T, size_t N> class Subtractive : public Minimizer<T>
	{
	public:
		using Minimizer<T>::voices, Minimizer<T>::overtones, Minimizer<T>::decay, Minimizer<T>::harmonicity,
			  Minimizer<T>::particles, Minimizer<T>::active, Minimizer<T>::pitches;

		using TN = std::array<T, N>;

		Subtractive() { }
		~Subtractive()
		{
			delete [] amplitudes;

			for (int i = 0; i < voices * overtones * N; i++)
				delete filters[i];
			delete [] filters;
		}

		Subtractive(uint voices, uint overtones, T decay, T resonance = 0.99999, T harmonicity = 1.0, T k = 0.1) : 
			Minimizer<T>(voices, overtones, decay, harmonicity), attack(relaxation(k)), resonance(resonance)
		{
			normalization = decay != 1 ? (1 - pow(decay, overtones)) / (1 - decay) : overtones;
			amplitudes = new T[voices];
			memset(amplitudes, 0, voices * sizeof(T));

			filters = new Filter<T>*[voices * overtones * N];
			for (int i = 0; i < voices * overtones * N; i++)
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
						while (frequency > SR / 2)
							frequency /= 2;

						for (int k = 0; k < N; k++)
						{
							filters[k * overtones * voices + i * overtones + j]->resonant(frequency, resonance);
							filters[k * overtones * voices + i * overtones + j]->tick();
						}
					}
		}

		TN operator()(const TN& sample)
		{
			TN out{};
			for (int i = 0; i < voices; i++)
				if (amplitudes[i])
					for (int j = 0; j < overtones; j++)
						for (int k = 0; k < N; k++)
							out[k] += amplitudes[i] * pow(decay, j) * ((*filters[k * overtones * voices + i * overtones + j])(sample[k])) / (voices * normalization);

			return out;
		}

	private:
		Filter<T>** filters;
		T* amplitudes;
		T normalization;
		T attack;

		T resonance;

		// clips (-infty, infty) to (-1, 1); linear in (-width, width)
		T softclip(T sample, T width = 0.99)
		{
			if (abs(sample) < width)
				return sample;

			int sign = sgn(sample);
			T gap = sample - sign * width;
			return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
		}

	};
#endif

#ifdef ORIGINAL
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

		Subtractive(uint voices, uint overtones, T decay, T resonance = 0.99999, T harmonicity = 1.0, T k = 0.1) : 
			Minimizer<T>(voices, overtones, decay, harmonicity), attack(relaxation(k)), resonance(resonance)
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
						while (frequency > SR / 2)
							frequency /= 2;

						filters[i * overtones + j]->resonant(frequency, resonance);
						filters[i * overtones + j]->tick();
					}
		}

		T operator()(T sample)
		{
			T out = 0;
			for (int i = 0; i < voices; i++)
				if (amplitudes[i])
					for (int j = 0; j < overtones; j++)
						// out += amplitudes[i] * softclip(pow(decay, j) * (*filters[i * overtones + j])(sample)) / (voices * normalization);
						out += amplitudes[i] * pow(decay, j) * ((*filters[i * overtones + j])(sample)) / (voices * normalization);

			return out;
		}

	private:
		Filter<T>** filters;
		T* amplitudes;
		T normalization;
		T attack;

		T resonance;

		// clips (-infty, infty) to (-1, 1); linear in (-width, width)
		T softclip(T sample, T width = 0.99)
		{
			if (abs(sample) < width)
				return sample;

			int sign = sgn(sample);
			T gap = sample - sign * width;
			return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
		}

	};
#endif

#ifdef ALLINONE
	// ALL IN ONE FILTERBANK
	template <typename T> class Subtractive : public Minimizer<T>
	{
	public:
		using Minimizer<T>::voices, Minimizer<T>::overtones, Minimizer<T>::decay, Minimizer<T>::harmonicity,
			  Minimizer<T>::particles, Minimizer<T>::active, Minimizer<T>::pitches;

		Subtractive() { }
		~Subtractive()
		{
			delete filters;
		}

		Subtractive(uint voices, uint overtones, T decay, T resonance = 0.99999, T harmonicity = 1.0, T k = 0.01) : 
			Minimizer<T>(voices, overtones, decay, harmonicity), attack(relaxation(k)), resonance(resonance)
		{
			normalization = decay != 1 ? (1 - pow(decay, overtones)) / (1 - decay) : overtones;

			filters = new Filterbank<T>(2, voices * overtones);
			for (int i = 0; i < voices * overtones; i++)
				filters->coefficients(i, {0,0,0}, {0});

			for (int i = 0; i < voices; i++)
				for (int j = 0; j < overtones; j++)
					filters->boost(i * overtones + j, pow(decay, j));

			filters->open();
		}

		void tick()
		{
			// update and tick filters
			for (int i = 0; i < voices; i++)
				for (int j = 0; j < overtones; j++)
				{
					T frequency = mtof(particles[i * overtones + j]());
					T cosine = cos(2 * PI * frequency / SR);
					T gain = resonant(frequency, resonance);

					filters->mix(i * overtones + j, active[i]);
					filters->coefficients(i * overtones + j, {gain, 0, -gain}, {-2 * resonance * cosine, resonance * resonance});
				}

			filters->tick();
		}

		T operator()(T sample)
		{
			return (*filters)(sample) / (voices * normalization);
		}

	private:
		Filterbank<T>* filters;
		T normalization;
		T attack;

		T resonance;

			// clips (-infty, infty) to (-1, 1); linear in (-width, width)
		T softclip(T sample, T width = 0.5)
		{
			if (abs(sample) < width)
				return sample;

			int sign = sgn(sample);
			T gap = sample - sign * width;
			return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
		}

		// returns scaling coefficient for two-pole, two-zero resonant filter
		T resonant(T frequency, T Q)
		{
			using namespace std::complex_literals;

			std::complex<T> cosine2(cos(4 * PI * frequency / SR), 0);
			std::complex<T> sine2(sin(4 * PI * frequency / SR), 0);
			
			std::complex<T> maximum = 1.0 / (Q - 1) - 1.0 / (Q - cosine2 - 1.0i * sine2);
			return 1 / sqrt(abs(maximum));
		}
	};
#endif


#ifdef ONEPERVOICE
	// ONE FILTERBANK PER VOICE
	template <typename T> class Subtractive : public Minimizer<T>
	{
	public:
		using Minimizer<T>::voices, Minimizer<T>::overtones, Minimizer<T>::decay, Minimizer<T>::harmonicity,
			  Minimizer<T>::particles, Minimizer<T>::active, Minimizer<T>::pitches;

		Subtractive() { }
		~Subtractive()
		{
			for (int i = 0; i < voices; i++)
				delete filters[i];
			delete [] filters;
		}

		Subtractive(uint voices, uint overtones, T decay, T resonance = 0.99999, T harmonicity = 1.0, T k = 0.1) : 
			Minimizer<T>(voices, overtones, decay, harmonicity), attack(relaxation(k)), resonance(resonance)
		{
			normalization = decay != 1 ? (1 - pow(decay, overtones)) / (1 - decay) : overtones;
			
			filters = new Filterbank<T>*[voices];
			for (int i = 0; i < voices; i++)
			{
				filters[i] = new Filterbank<T>(2, overtones);
				for (int j = 0; j < overtones; j++)
					filters[i]->mix(j, pow(decay, j));
			}
		}

		void tick()
		{
			for (int i = 0; i < voices; i++)
				for (int j = 0; j < overtones; j++)
					filters[i]->boost(j, active[i]);


			// update and tick filters
			for (int i = 0; i < voices; i++)
				for (int j = 0; j < overtones; j++)
				{
					T frequency = mtof(particles[i * overtones + j]());
					T cosine = cos(2 * PI * frequency / SR);
					T gain = resonant(frequency, resonance);

					filters[i]->coefficients(j, {gain, 0, -gain}, {-2 * resonance * cosine, resonance * resonance});
					filters[i]->tick();
				}
		}

		T operator()(T sample)
		{
			T out = 0;
			for (int i = 0; i < voices; i++)
				out += (*filters[i])(sample);

			return out / (voices * normalization);
		}

	private:
		Filterbank<T>** filters;
		T normalization;
		T attack;

		T resonance;

		// clips (-infty, infty) to (-1, 1); linear in (-width, width)
		T softclip(T sample, T width = 0.5)
		{
			if (abs(sample) < width)
				return sample;

			int sign = sgn(sample);
			T gap = sample - sign * width;
			return sign * width + (1 - width) * 2.0 / PI * atan(PI * gap / (2 * (1 - width)));
		}

		// returns scaling coefficient for two-pole, two-zero resonant filter
		T resonant(T frequency, T Q)
		{
			using namespace std::complex_literals;

			std::complex<T> cosine2(cos(4 * PI * frequency / SR), 0);
			std::complex<T> sine2(sin(4 * PI * frequency / SR), 0);
			
			std::complex<T> maximum = 1.0 / (Q - 1) - 1.0 / (Q - cosine2 - 1.0i * sine2);
			return 1 / sqrt(abs(maximum));
		}

	};
#endif

}

#define SUBTRACTIVE
#endif