// bowl.h
#include "includes.h"
#include "slidebank.h"
#include "mixer.h"
#include "signal.h"

#ifndef BOWL

namespace soundmath
{
	// a bank of synthesizers, with optional harmonicity.
	template <typename T, int overtones> class Bowl
	{
	public:
		Bowl() { }
		~Bowl()
		{
			delete sb;
			delete impulse;
			delete zeros;
		}

		Bowl(const std::vector<T>& frequencies, const std::vector<T>& amplitudes, const std::vector<T>& decays, Wave<T>* form = &cycle) : 
			form(form), overtones(overtones), frequencies(frequencies), amplitudes(amplitudes), decays(decays)
		{
			zeros = new Signal<std::complex<T>, overtones>;
			impulse = new Signal<std::complex<T>, overtones>;
			std::vector<std::complex<T>> ones(overtones, 1);
			impulse(ones);

			std::vector<std::complex<T>> radii(overtones);
			for (int i = 0; i < overtones; i++)
			{
				T 
				radii[i] = std::exp(-decays[i] / SR) * std::complex<T>()
			}
			sb = new Slidebank<T, overtones>(1,);


			// overtones = frequencies.size();
			this->frequencies.resize(overtones, 0);
			this->amplitudes.resize(overtones, 0);
			this->decays.resize(overtones, 0);
		}

		void trigger()
		{
			phase = 0;
		}

		T operator()()
		{
			if (!computed)
			{
				sample = 0;
				for (int i = 0; i < overtones; i++)
					sample += amplitudes[i] * pow(E, -decays[i] * phase / SR) * (*form)(frequencies[i] * phase / SR);

				computed = true;
			}

			return sample;
		}

		void tick()
		{
			phase++;
			computed = false;
		}

		int fill(float* buffer, int bsize)
		{
			for (int j = 0; j < bsize; j++)
			{
				sample = 0;
				for (int i = 0; i < overtones; i++)
					sample += amplitudes[i] * pow(E, -decays[i] * phase / SR) * (*form)(frequencies[i] * phase / SR);
				
				phase++;
				buffer[j] = sample;
			}

			return 0;
		}

	private:
		Wave<T>* form;

		Slidebank<T, overtones>* sb;
		Signal<T, overtones>* impulse;
		Signal<T, overtones>* zeros;

		std::vector<T> frequencies;
		std::vector<T> amplitudes;
		std::vector<T> decays;
		bool computed = false;
		T sample = 0;
		T phase = 0;
	};
}

#define BOWL
#endif