// fourier.h
#include "includes.h"
#include "wave.h"
#include "delay.h"
#include <fftw3.h>

#ifndef FOURIER

namespace soundmath
{
	// class Fourier
	// {
	// public:	
	// 	// initialize a DFT of size n
	// 	Fourier(int N, fftw_complex** user_in, fftw_complex** user_out)
	// 	{
	// 		in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
	// 		out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

	// 		*user_in = in;
	// 		*user_out = out;

	// 		p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);
	// 		q = fftw_plan_dft_1d(N, out, in, FFTW_BACKWARD, FFTW_MEASURE);
	// 	}

	// 	~Fourier()
	// 	{
	// 		fftw_destroy_plan(p);
	// 		fftw_destroy_plan(q);
	// 		fftw_free(in); fftw_free(out);
	// 	}

	// 	void forward()
	// 	{
	// 		fftw_execute(p);
	// 	}

	// 	void backward()
	// 	{
	// 		fftw_execute(q);
	// 	}

	// private:
	// 	fftw_complex *in, *out;
	// 	fftw_plan p, q;
	// };


	class Fourier
	{
	public:
		int (*processor)(const std::complex<double>* in, std::complex<double>* out);

		Fourier(int (*processor)(const std::complex<double>*, std::complex<double>*), int N, int laps) : processor(processor), N(N), laps(laps), stride(N / laps)
		{
			in = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N * laps * 2);
			middle = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N * laps * 2);
			out = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N * laps * 2); // lookup with out[j + N * i]

			analysis = new fftw_plan[laps * 2];
			synthesis = new fftw_plan[laps * 2];

			for (int i = 0; i < laps * 2; i++)
			{
				analysis[i] = fftw_plan_dft_1d(N, reinterpret_cast<fftw_complex*>(in + i * N), reinterpret_cast<fftw_complex*>(middle + i * N), FFTW_FORWARD, FFTW_MEASURE);
				synthesis[i] = fftw_plan_dft_1d(N, reinterpret_cast<fftw_complex*>(out + i * N), reinterpret_cast<fftw_complex*>(in + i * N), FFTW_BACKWARD, FFTW_MEASURE);
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
		}

		~Fourier()
		{
			for (int i = 0; i < laps * 2; i++)
			{
				fftw_destroy_plan(analysis[i]);
				fftw_destroy_plan(synthesis[i]);
			}
			fftw_free(in); fftw_free(middle); fftw_free(out);

			delete [] writepoints;
			delete [] readpoints;
			delete [] reading;
			delete [] writing;
		}

		// writes a single sample (with windowing) into the in array
		void write(double real, double imag = 0)
		{
			for (int i = 0; i < laps * 2; i++)
			{
				if (writing[i])
				{
					if (writepoints[i] >= 0)
					{
						double window = halfhann(writepoints[i] / (double)N);
						in[writepoints[i] + N * i].real(window * real);
						in[writepoints[i] + N * i].imag(window * imag);
					}
					writepoints[i]++;

					if (writepoints[i] == N)
					{
						writing[i] = false;
						reading[i] = true;
						readpoints[i] = 0;

						forward(i); // FTs ith in to ith middle buffer
						process(i); // user-defined; ought to move info from ith middle to out buffer 
						backward(i); // IFTs ith out to ith in buffer
					}
				}
			}
		}

		inline void forward(const int i)
		{
			fftw_execute(analysis[i]);
		}

		inline void backward(const int i)
		{
			fftw_execute(synthesis[i]);
		}

		// executes user-defined callback
		inline void process(const int i)
		{
			processor((middle + i * N), (out + i * N));
		}

		// read a single reconstructed sample (real and imaginary parts)
		void read(double* real, double* imag)
		{
			long double realaccum = 0;
			long double imagaccum = 0;

			for (int i = 0; i < laps * 2; i++)
			{
				if (reading[i])
				{
					double window = halfhann(readpoints[i] / (double)N);
					std::complex<double> sample = in[readpoints[i] + N * i];
					realaccum += window * sample.real();
					imagaccum += window * sample.imag();

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
		std::complex<double> *in, *middle, *out;

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


	class Cosine
	{
	public:
		// initialize a DCT of size n
		Cosine(int N, double** user_in, double** user_out)
		{
			in = (double*) fftw_malloc(sizeof(double) * N);
			out = (double*) fftw_malloc(sizeof(double) * N);

			*user_in = in;
			*user_out = out;

			p = fftw_plan_r2r_1d(N, in, out, FFTW_REDFT10, FFTW_MEASURE);
			q = fftw_plan_r2r_1d(N, out, in, FFTW_REDFT01, FFTW_MEASURE);
		}

		~Cosine()
		{
			fftw_destroy_plan(p);
			fftw_destroy_plan(q);
			fftw_free(in); fftw_free(out);
		}

		void forward()
		{
			fftw_execute(p);
		}

		void backward()
		{
			fftw_execute(q);
		}

	private:
		double *in, *out;
		fftw_plan p, q;

	};

	template <int N> class FFrame
	{
	public:
		FFrame()
		{
			data = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N);
			frequencies = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N);

			memset(data, 0, sizeof(std::complex<double>) * N);
			memset(frequencies, 0, sizeof(std::complex<double>) * N);

			analysis = fftw_plan_dft_1d(N, reinterpret_cast<fftw_complex*>(data), reinterpret_cast<fftw_complex*>(frequencies), FFTW_FORWARD, FFTW_MEASURE);

			norms = new double[N];
			phases = new double[N];
		}

		~FFrame()
		{
			fftw_destroy_plan(analysis);

			fftw_free(data);
			fftw_free(frequencies);

			delete [] norms;
			delete [] phases;
		}

		// writes a (windowed) value at a given index in {0, ..., N-1}
		void write(int index, double real, double imag)
		{
			index %= N;
			data[index].real(halfhann(index / (double)N) * real);
			data[index].imag(halfhann(index / (double)N) * imag);
		}

		// computes FFT of data array, writes to frequencies array
		void process()
		{
			fftw_execute(analysis);
		}

		// computes polar representation of frequencies array, stores to norms and phases arrays
		void polarize()
		{
			for (int i = 0; i < N; i++)
			{
				norms[i] = std::norm(frequencies[i]);
				phases[i] = std::arg(frequencies[i]);
			}
		}

	public:
		double* norms;
		double* phases;

	private:
		std::complex<double>* data;
		std::complex<double>* frequencies;
		
		fftw_plan analysis;
	};


	template <int N> class IFrame
	{
	public:
		IFrame()
		{
			data = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N);
			frequencies = (std::complex<double>*) fftw_malloc(sizeof(std::complex<double>) * N);

			memset(data, 0, sizeof(std::complex<double>) * N);
			memset(frequencies, 0, sizeof(std::complex<double>) * N);

			synthesis = fftw_plan_dft_1d(N, reinterpret_cast<fftw_complex*>(frequencies), reinterpret_cast<fftw_complex*>(data), FFTW_BACKWARD, FFTW_MEASURE);
		}

		~IFrame()
		{
			fftw_destroy_plan(synthesis);

			fftw_free(data);
			fftw_free(frequencies);
		}

		// returns a (windowed) value at a given index in {0, ..., N-1}
		std::complex<double> read(int index)
		{
			return data[index] * halfhann(index / (double)N);
		}

		void populate(double* norms, double* phases)
		{
			for (int i = 0; i < N; i++)
			{
				frequencies[i] = std::polar(std::sqrt(norms[i]), phases[i]);
			}
		}

		// computes FFT of data array, writes to frequencies array
		void process()
		{
			fftw_execute(synthesis);
		}

	private:
		std::complex<double>* data;
		std::complex<double>* frequencies;
		
		fftw_plan synthesis;
	};


	template <int N> class DFrame
	{
	public:
		DFrame()
		{
			norms = new double[N];
			phasechange = new double[N];
		}

		~DFrame()
		{
			delete [] norms;
			delete [] phasechange;
		}

		void bind(FFrame<N>* first, FFrame<N>* second)
		{
			this->first = first;
			this->second = second;
		}

		void populate()
		{
			memcpy(norms, second->norms, N * sizeof(double));
			for (int i = 0; i < N; i++)
			{
				phasechange[i] = second->phases[i] - first->phases[i];
			}
		}

	private:
		FFrame<N> *first, *second;


	public:
		double* norms; // norms in second (should this be average norm?)
		double* phasechange; // second.phase - first.phase
	};

	template <int N> class Freezer
	{
	public:
		// initialize frames of size N with overlap factor laps
		// choose the number of windows so that total circular buffer length is approx width * N
		// (width >= 1)
		Freezer(int laps, double width) : origin(0), readhead(0)
		{
			width = std::max(width, 1.0); // require width >= 1
			laps = std::max(laps, 2); // require at least overlap factor of 2
			stride = N / laps; // okay if laps doesn't divide N
			M = (int)(width * laps) + 1; // total number of buffers
			size = M * stride;
			readsize = size;

			frames = new FFrame<N>*[M];
			inverses = new IFrame<N>*[M];
			differences = new DFrame<N>*[M];

			for (int i = 0; i < M; i++)
			{
				frames[i] = new FFrame<N>();
				inverses[i] = new IFrame<N>();
				differences[i] = new DFrame<N>();
			}

			for (int i = 0; i < M; i++)
				differences[i]->bind(frames[i], frames[(i + 1) % M]);

			vocoder = new double[N];
			memset(vocoder, 0, sizeof(double) * N);

			iqueue = new int[M];
			memset(iqueue, 0, sizeof(int) * M);
			iindex = 0;

			delay = new Delay<double>(1, N);
			delay->coefficients({{N,1}}, {}); // delay with gain 1 by N samples
		}

		~Freezer()
		{
			for (int i = 0; i < M; i++)
			{
				delete frames[i];
				delete inverses[i];
				delete differences[i];
			}

			delete [] frames;
			delete [] inverses;
			delete [] differences;

			delete [] vocoder;
			delete [] iqueue;

			delete delay;
		}

		void write(double sample)
		{
			// write into all active frames
			for (int i = 0; i < M; i++) // could be improved eventually
			{
				int spot = (origin - i * stride + size) % size; // offset from frame start
				if (spot < N)
					frames[i]->write(spot, sample, 0);

				if (spot == 0) // finished writing to frame
				{
					frames[(M + i - 1) % M]->process();
					frames[(M + i - 1) % M]->polarize();
				}
			}

			origin++;
			origin %= size;
		}

		void freeze()
		{
			if (!frozen)
			{
				excluded = origin / stride; // exclude DFrames adjacent to this one
				for (int i = 1; i < M - 1; i++)
					differences[(excluded + i) % M]->populate();
			}

			readhead = 0;
			frozen = true;
		}

		void unfreeze()
		{
			frozen = false;
			memset(vocoder, 0, sizeof(double) * N);
		}

		double operator()(double sample)
		{
			write(sample);
			double output = 0;
			if (frozen)
			{
				for (int i = 0; i < M; i++) // iterate through iqueue
				{
					int spot = (readhead - i * stride + readsize) % readsize; // offset from most recent frame start
					if (spot < N) // if inverses[iqueue[i]] is in scope
					{
						output += std::real(inverses[iqueue[i]]->read(spot));
					}

					if (spot == 0) // this means readhead == i * stride; we need a new iframe
					{
						int next = std::rand() % (M - 2); // next fframe to work on
						if (next >= excluded)
							next += 2;

						iindex++;
						iindex %= M;
						iqueue[iindex] = next;
						// std::cout << next << std::endl;

						for (int j = 0; j < N; j++)
						{
							vocoder[j] = differences[next]->phasechange[j];
							vocoder[j] = std::fmod(vocoder[j], 2 * PI);
						}

						inverses[next]->populate(differences[next]->norms, vocoder);
						inverses[next]->process();
					}
				}

				readhead++;
				readhead %= readsize;

				output /= N;
			}
			else
			{
				output = (*delay)(sample);
			}

			delay->tick();

			return output;
		}


	private:
		FFrame<N>** frames;
		IFrame<N>** inverses;
		DFrame<N>** differences;

		Delay<double>* delay;

		double* vocoder; // phase accumulation

		int origin; // writehead
		int readhead; // readhead
		int stride;
		int laps;
		int M;
		int size;

		int excluded;
		int* iqueue; // queue of IFrames
		int iindex; // index in iqueue
		int readsize;

		double mix; // 0 = dry, 1 = frozen
		bool frozen;
	};


}

#define FOURIER
#endif