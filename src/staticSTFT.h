// fourier.h
#include "includes.h"
#include "wave.h"
#include <fftw3.h>

#ifndef STATICFOURIER

namespace soundmath
{
	class StaticSTFT
	{
	public:
		StaticSTFT(int N, int laps) : N(N), laps(laps), stride(N / laps)
		{
			in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * laps * 2);
			out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * laps * 2); // lookup with out[j + N * i]

			analysis = new fftw_plan[laps * 2];
			synthesis = new fftw_plan[laps * 2];

			for (int i = 0; i < laps * 2; i++)
			{
				analysis[i] = fftw_plan_dft_1d(N, in + i * N, out + i * N, FFTW_FORWARD, FFTW_MEASURE);
				synthesis[i] = fftw_plan_dft_1d(N, out + i * N, in + i * N, FFTW_BACKWARD, FFTW_MEASURE);
			}

			writepoints = new int[laps * 2];
			readpoints = new int[laps * 2];

			memset(writepoints, 0, sizeof(int) * laps * 2);
			memset(readpoints, 0, sizeof(int) * laps * 2);

			for (int i = 0; i < 2 * laps; i++) // initialize half of writepoints
				writepoints[i] = -stride * i;

			reading = new bool[laps * 2];
			writing = new bool[laps * 2];

			memset(reading, false, sizeof(bool) * laps * 2);
			memset(writing, true, sizeof(bool) * laps * 2);
			// for (int i = 0; i < laps; i++) // set half of writing to true
			// 	writing[i] = true;
		}

		~StaticSTFT()
		{
			for (int i = 0; i < laps * 2; i++)
			{
				fftw_destroy_plan(analysis[i]);
				fftw_destroy_plan(synthesis[i]);
			}
			fftw_free(in); fftw_free(out);

			delete [] writepoints;
			delete [] readpoints;
			delete [] reading;
			delete [] writing;
		}

		void write(double real, double imag = 0)
		{
			for (int i = 0; i < laps * 2; i++)
			{
				if (writing[i])
				{
					if (writepoints[i] >= 0)
					{
						double window = hann(writepoints[i] / (double)N);
						in[writepoints[i] + N * i][0] = window * real;
						in[writepoints[i] + N * i][1] = window * imag;
					}
					writepoints[i]++;

					if (writepoints[i] == N)
					{
						writing[i] = false;
						reading[i] = true;
						readpoints[i] = 0;

						forward(i);
						process(i);
						backward(i);
					}
				}
			}
		}

		void forward(int i)
		{
			fftw_execute(analysis[i]);
		}

		void backward(int i)
		{
			fftw_execute(synthesis[i]);
		}

		// do something in the frequency domain (should this be a callback??)
		void process(int i)
		{
			double average = 0;
			double bin[2];
			for (int j = 0; j < N; j++)
			{
				bin[0] = out[j + i * N][0];
				bin[1] = out[j + i * N][1];
				average += sqrt(bin[0] * bin[0] + bin[1] * bin[1]) / N;
			}

			for (int j = 0; j < N; j++)
			{
				bin[0] = out[j + i * N][0];
				bin[1] = out[j + i * N][1];
				if (bin[0] * bin[0] + bin[1] * bin[1] < 100 * average * average)
				{
					out[j + i * N][0] *= 0.1;
					out[j + i * N][1] *= 0.1;
				}
				else
				{
					// out[(j * 2) % N + i * N][0] += bin[0] / 2.0;
					// out[(j * 2) % N + i * N][1] += bin[1] / 2.0;	

					// out[(j / 2) % N + i * N][0] -= bin[0] / 2.0;
					// out[(j / 2) % N + i * N][1] -= bin[1] / 2.0;
				}
			}
		}

		void read(double* real, double* imag)
		{
			long double realaccum = 0;
			long double imagaccum = 0;

			for (int i = 0; i < laps * 2; i++)
			{
				if (reading[i])
				{
					double window = hann(readpoints[i] / (double)N);
					double* sample = in[readpoints[i] + N * i];
					realaccum += window * sample[0];
					imagaccum += window * sample[1];

					readpoints[i]++;

					if (readpoints[i] == N)
					{
						writing[i] = true;
						reading[i] = false;
						writepoints[i] = 0;
					}
				}
			}

			realaccum /= N * laps / 2;
			imagaccum /= N * laps / 2;

			*real = realaccum;
			*imag = imagaccum;
		}



	private:
		fftw_complex *in, *out;

		int N;
		int laps;
		int stride;

		fftw_plan* analysis;
		fftw_plan* synthesis;
		int* writepoints;
		int* readpoints;
		bool* reading;
		bool* writing;
	};
}

#define STATICFOURIER
#endif