// filterbank.h
#include "includes.h"
#include "buffer.h"

using Eigen::Matrix;
using Eigen::Dynamic;
using Eigen::Map;
using Eigen::seqN;
using Eigen::Array;

#ifndef FILTERBANK

namespace soundmath
{
	// many filters in parallel
	template <typename T> class Filterbank
	{
		typedef Matrix<T, Dynamic, Dynamic> MatrixT;
		typedef Matrix<T, Dynamic, 1> VectorT;

	public:
		Filterbank() { }
		~Filterbank()
		{
			delete forwards;
			delete backs;
			delete input;
			delete outputs;
			delete preamps_in;
			delete preamps_out;
			delete gains_in;
			delete gains_out;
		}

		// initialize N filters of a given order, reading and working with circular buffers
		Filterbank(int order, int N = 1, double k_p = 0.1, double k_g = 1) : N(N), order(order), smoothing_p(relaxation(k_p)),
																								 smoothing_g(relaxation(k_g))
		{
			forwards = new MatrixT;
			backs = new MatrixT;
			input = new VectorT;
			outputs = new MatrixT;

			preamps_in = new VectorT;
			preamps_out = new VectorT; // amount of input signal passed to the filter
			gains_in = new VectorT;
			gains_out = new VectorT; // amount of output signal in mixdown

			forwards->resize(N, order + 1);
			backs->resize(N, order);
			input->resize(2 * (order + 1)); // allows for circular buffering without modulo
			outputs->resize(2 * (order + 1), N); // N circular buffers of output
			
			preamps_in->resize(N);
			preamps_out->resize(N);
			
			gains_in->resize(N);
			gains_out->resize(N);

			forwards->setZero();
			backs->setZero();
			input->setZero();
			outputs->setZero();
			
			preamps_in->setZero();
			preamps_out->setZero();

			gains_in->setZero();
			gains_out->setZero();
		}

		// initalize the nth filter's coefficients
		void coefficients(int n, const std::vector<T>& forward, const std::vector<T>& back)
		{
			int coeffs = std::min<int>(order + 1, forward.size());
			for (int i = 0; i < coeffs; i++)
				(*forwards)(n, i) = forward[i];

			coeffs = std::min<int>(order, back.size());
			for (int i = 0; i < coeffs; i++)
				(*backs)(n, i) = back[i];
		}

		// set nth filter's preamp coefficient
		void boost(int n, T value)
		{
			(*preamps_in)(n) = value;
		}

		// set all preamps_out coefficients
		void boost(const std::vector<T>& values)
		{
			for (int i = 0; i < std::min<int>(N, values.size()); i++)
				(*preamps_in)(i) = values[i];

		}

		// set nth filter's mixdown coefficient
		void mix(int n, T value)
		{
			(*gains_in)(n) = value;
		}

		// set all mixdown coefficients
		void mix(const std::vector<T>& values)
		{
			for (int i = 0; i < std::min<int>(N, values.size()); i++)
				(*gains_in)(i) = values[i];

		}

		void open()
		{
			for (int i = 0; i < N; i++)
				(*gains_in)(i) = 1;
		}

		void print()
		{
			std::cout << "forwards = \n" << *forwards << std::endl;
			std::cout << "backs = \n" << *backs << std::endl;
		}

		// get the result of filters applied to a sample
		T operator()(T sample)
		{
			if (!computed)
				compute(sample);

			return outputs->row(origin) * (*gains_out);
		}

		T operator()(T sample, T (*distortion)(T in))
		{
			if (!computed)
				compute(sample);

			return ((outputs->row(origin).transpose().array() * (*gains_out).array()).unaryExpr(distortion)).sum();
		}

		// timestep
		void tick()
		{
			origin--;
			if (origin < 0)
				origin += order + 1;
			computed = false;
		}

	private:
		MatrixT* forwards; // feedforward coeffs
		MatrixT* backs; // feedback coeffs

		VectorT* input;
		MatrixT* outputs;

		VectorT* preamps_in;
		VectorT* preamps_out;

		VectorT* gains_in;
		VectorT* gains_out;

		int N; // number of filters
		int order; // highest order of filters involved
		double smoothing_p, smoothing_g;

		int origin = 0;
		bool computed = false; // flag in case of repeated calls to operator()

		void compute(T sample)
		{
			*preamps_out = (1 - smoothing_p) * (*preamps_in) + smoothing_p * (*preamps_out);
			*gains_out = (1 - smoothing_g) * (*gains_in) + smoothing_g * (*gains_out);

			(*input)(origin) = sample;
			(*input)(origin + (order + 1)) = sample;

			VectorT temp = (((*forwards) * (*input)(seqN(origin, order + 1))).array() * preamps_out->array()).matrix()
						 - ((*backs) * outputs->block(origin + 1, 0, order, N)).diagonal();

			// temp = temp.array() * preamps_out->array();

			outputs->row(origin) = temp;
			outputs->row(origin + (order + 1)) = temp;

			computed = true;
		}
	};

	// preallocated filterbank
	template <typename T, size_t N, size_t order> class FFilterbank
	{
		typedef Matrix<T, N, order + 1> ForwardMatrix;
		typedef Matrix<T, N, order> BackMatrix;
		typedef Matrix<T, 2 * (order + 1), 1> InputVector;
		typedef Matrix<T, 2 * (order + 1), N> OutputMatrix;
		typedef Matrix<T, N, 1> MixerVector;

	public:
		// initialize N filters of a given order, reading and working with circular buffers
		FFilterbank(double k_p = 0.1, double k_g = 1) : 
			smoothing_p(relaxation(k_p)), smoothing_g(relaxation(k_g))
		{
			forwards.setZero();
			backs.setZero();
			input.setZero();
			outputs.setZero();
			
			preamps_in.setZero();
			preamps_out.setZero();

			gains_in.setZero();
			gains_out.setZero();
		}

		~FFilterbank() { }

		// initalize the nth filter's coefficients
		void coefficients(int n, const std::vector<T>& forward, const std::vector<T>& back)
		{
			int coeffs = std::min<int>(order + 1, forward.size());
			for (int i = 0; i < coeffs; i++)
				forwards(n, i) = forward[i];

			coeffs = std::min<int>(order, back.size());
			for (int i = 0; i < coeffs; i++)
				backs(n, i) = back[i];
		}

		// set nth filter's preamp coefficient
		void boost(int n, T value)
		{
			preamps_in(n) = value;
		}

		// set all preamps_out coefficients
		void boost(const std::vector<T>& values)
		{
			for (int i = 0; i < std::min<int>(N, values.size()); i++)
				preamps_in(i) = values[i];

		}

		// set nth filter's mixdown coefficient
		void mix(int n, T value)
		{
			gains_in(n) = value;
		}

		// set all mixdown coefficients
		void mix(const std::vector<T>& values)
		{
			for (int i = 0; i < std::min<int>(N, values.size()); i++)
				gains_in(i) = values[i];

		}

		void open()
		{
			for (int i = 0; i < N; i++)
				gains_in(i) = 1;
		}

		// get the result of filters applied to a sample
		T operator()(T sample)
		{
			if (!computed)
				compute(sample);

			return outputs.row(origin) * gains_out;
		}

		T operator()(T sample, T (*distortion)(T in))
		{
			if (!computed)
				compute(sample);

			return ((outputs.row(origin).transpose().array() * gains_out.array()).unaryExpr(distortion)).sum();
		}

		// timestep
		void tick()
		{
			origin--;
			if (origin < 0)
				origin += order + 1;
			computed = false;
		}

	private:
		T smoothing_p, smoothing_g;

		ForwardMatrix forwards;
		BackMatrix backs;
		InputVector input;
		OutputMatrix outputs;

		MixerVector preamps_in, preamps_out, gains_in, gains_out;

		int origin = 0;
		bool computed = false; // flag in case of repeated calls to operator()

		void compute(T sample)
		{
			preamps_out = (1 - smoothing_p) * preamps_in + smoothing_p * preamps_out;
			gains_out = (1 - smoothing_g) * gains_in + smoothing_g * gains_out;

			input(origin) = sample;
			input(origin + (order + 1)) = sample;

			MixerVector temp = ((forwards * input(seqN(origin, order + 1))).array() * preamps_out.array()).matrix()
						 - (backs * outputs.block(origin + 1, 0, order, N)).diagonal();

			outputs.row(origin) = temp;
			outputs.row(origin + (order + 1)) = temp;

			computed = true;
		}
	};


	// // many filters in parallel
	// template <typename T> class Filterbank
	// {
	// public:
	// 	Filterbank() { }
	// 	~Filterbank()
	// 	{
	// 		delete [] forwards;
	// 		delete [] backs;
	// 		delete [] outputs;
	// 		delete [] gains_out;
	// 	}

	// 	// initialize N filters of a given order, reading and working with circular buffers
	// 	Filterbank(int order, int N = 1) : input(order + 2)
	// 	{
	// 		outputs = new Buffer<T>[N];
	// 		for (int i = 0; i < N; i++)
	// 			outputs[i].initialize(order + 2);

	// 		gains_out = new T[N];
	// 		memset(gains_out, 0, N * sizeof(T));

	// 		forwards = new T[(order + 1) * N];
	// 		backs = new T[(order + 1) * N];

	// 		memset(forwards, 0, (order + 1) * N * sizeof(T));
	// 		memset(backs, 0, (order + 1) * N * sizeof(T));

	// 		this->order = order;
	// 		this->N = N;
	// 		computed = false;
	// 	}

	// 	// initalize the nth filter's coefficients
	// 	void coefficients(T gain, const std::vector<T>& forward, const std::vector<T>& back, int n = 0)
	// 	{
	// 		for (int i = 0; i < order + 1; i++)
	// 		{
	// 			forwards[i * N + n] = forward[i];
	// 			backs[i * N + n] = back[i];
	// 		}
	// 		backs[n] = 0;
	// 		gains_out[n] = gain;
	// 	}

	// 	// get the result of filters applied to a sample
	// 	T operator()(T sample)
	// 	{
	// 		if (!computed)
	// 		{
	// 			T out = 0;
	// 			input.write(sample);

	// 			for (int j = 0; j < N; j++)     		// j is filter number
	// 			{
	// 				outputs[j].write(0);
	// 				for (int i = 0; i < order + 1; i++) // i is delay time
	// 					outputs[j].accum(forwards[i * N + j] * input(i) - backs[i * N + j] * outputs[j](i));

	// 				out += gains_out[j] * outputs[j]();
	// 			}

	// 			computed = true;
	// 			return out;
	// 		}

	// 		T out = 0;
	// 		for (int j = 0; j < N; j++)
	// 			out += gains_out[j] * outputs[j]();

	// 		return out;
	// 	}

	// 	// timestep
	// 	void tick()
	// 	{
	// 		input.tick();
	// 		for (int j = 0; j < N; j++)
	// 			outputs[j].tick();
	// 		computed = false;
	// 	}

	// private:
	// 	Buffer<T> input; // circular buffers of inputs and outputs
	// 	Buffer<T>* outputs;
	// 	T* gains_out;

	// 	int N; // number of filters
	// 	int order;

	// 	T* forwards; // feedforward coefficient lists
	// 	T* backs; // feedback coefficient lists

	// 	bool computed; // flag in case of repeated calls to operator()
	// };


	// template <typename T> class FilterType
	// {
	// public:
	// 	FilterType() { }
	// 	~FilterType() { }

	// 	// zeros at DC and Nyquist, conjugate poles at a specified frequency and modulus
	// 	static std::pair<std::vector<T>, std::vector<T>> resonant(T gain, T frequency, T modulus)
	// 	{
	// 		using namespace std::complex_literals;
						
	// 		T cosine = cos(2 * PI * frequency / SR);

	// 		std::complex<T> cosine2(cos(4 * PI * frequency / SR), 0);
	// 		std::complex<T> sine2(sin(4 * PI * frequency / SR), 0);
			
	// 		std::complex<T> maximum = 1.0 / (modulus - 1) - 1.0 / (modulus - cosine2 - 1.0i * sine2);
	// 		T amplitude = gain / sqrt(abs(maximum));

	// 		std::vector<T> forward({amplitude, 0, -amplitude});
	// 		std::vector<T> back({0, -2 * modulus * cosine, modulus * modulus});

	// 		return {forward, back};
	// 	}
	// };
}

#define FILTERBANK
#endif