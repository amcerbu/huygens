// filterbank.h
#include "includes.h"
#include "buffer.h"

#ifndef FILTERBANK

namespace soundmath
{	
	// many filters in parallel
	template <typename T> class Filterbank
	{
	public:
		Filterbank() { }
		~Filterbank()
		{
			delete [] forwards;
			delete [] backs;
			delete [] outputs;
			delete [] gains;
		}

		// initialize N filters of a given order, reading and working with circular buffers
		Filterbank(uint order, uint N = 1) : input(order + 2)
		{
			outputs = new Buffer<T>[N];
			for (int i = 0; i < N; i++)
				outputs[i].initialize(order + 2);

			gains = new T[N];
			memset(gains, 0, N * sizeof(T));

			forwards = new T[(order + 1) * N];
			backs = new T[(order + 1) * N];

			memset(forwards, 0, (order + 1) * N * sizeof(T));
			memset(backs, 0, (order + 1) * N * sizeof(T));

			this->order = order;
			this->N = N;
			computed = false;
		}

		// initalize the nth filter's coefficients
		void coefficients(T gain, const std::vector<T>& forward, const std::vector<T>& back, uint n = 0)
		{
			for (int i = 0; i < order + 1; i++)
			{
				forwards[i * N + n] = forward[i];
				backs[i * N + n] = back[i];
			}
			backs[n] = 0;
			gains[n] = gain;
		}

		// get the result of filters applied to a sample
		T operator()(T sample)
		{
			if (!computed)
			{
				T out = 0;
				input.write(sample);

				for (int j = 0; j < N; j++)     		// j is filter number
				{
					outputs[j].write(0);
					for (int i = 0; i < order + 1; i++) // i is delay time
						outputs[j].accum(forwards[i * N + j] * input(i) - backs[i * N + j] * outputs[j](i));

					out += gains[j] * outputs[j]();
				}

				computed = true;
				return out;
			}

			T out = 0;
			for (int j = 0; j < N; j++)
				out += gains[j] * outputs[j]();

			return out;
		}

		// timestep
		void tick()
		{
			input.tick();
			for (int j = 0; j < N; j++)
				outputs[j].tick();
			computed = false;
		}

	private:
		Buffer<T> input; // circular buffers of inputs and outputs
		Buffer<T>* outputs;
		T* gains;

		uint N; // number of filters
		uint order;

		T* forwards; // feedforward coefficient lists
		T* backs; // feedback coefficient lists

		bool computed; // flag in case of repeated calls to operator()
	};


	template <typename T> class FilterType
	{
	public:
		FilterType() { }
		~FilterType() { }

		// zeros at DC and Nyquist, conjugate poles at a specified frequency and modulus
		static std::pair<std::vector<T>, std::vector<T>> resonant(T gain, T frequency, T modulus)
		{
			using namespace std::complex_literals;
						
			T cosine = cos(2 * PI * frequency / SR);

			std::complex<T> cosine2(cos(4 * PI * frequency / SR), 0);
			std::complex<T> sine2(sin(4 * PI * frequency / SR), 0);
			
			std::complex<T> maximum = 1.0 / (modulus - 1) - 1.0 / (modulus - cosine2 - 1.0i * sine2);
			T amplitude = gain / sqrt(abs(maximum));

			std::vector<T> forward({amplitude, 0, -amplitude});
			std::vector<T> back({0, -2 * modulus * cosine, modulus * modulus});

			return {forward, back};
		}
	};
}

#define FILTERBANK
#endif