// network.h
#include "includes.h"
#include "buffer.h"

using Eigen::Matrix;
using Eigen::Dynamic;
using Eigen::Map;
using Eigen::seqN;
using Eigen::Array;

#ifndef NETWORK

namespace soundmath
{
	// modulation / demodulation network
	template <typename T> class Network
	{
		typedef Matrix<T, Dynamic, Dynamic> MatrixT;
		typedef Matrix<T, Dynamic, 1> VectorT;
		typedef Array<T, Dynamic, 1> ArrayT;

	public:
		Network() { }
		~Network()
		{
			delete forwards;
			delete backs;
			delete input;
			delete outputs;
			delete preamps_in;
			delete preamps_out;
			delete gains_in;
			delete gains_out;
			delete modulator;
			delete demodulator;
		}

		// initialize N DC-pass filters of a given order, reading and working with circular buffers
		Network(int order, int N = 1, double k_p = 0.1, double k_g = 1) : N(N), order(order), smoothing_p(relaxation(k_p)),
																								 smoothing_g(relaxation(k_g))
		{
			forwards = new VectorT;
			backs = new VectorT;
			input = new MatrixT;
			outputs = new MatrixT;

			modulator = new ArrayT;
			demodulator = new ArrayT;

			preamps_in = new VectorT;
			preamps_out = new VectorT; // amount of input signal passed to the filter
			gains_in = new VectorT;
			gains_out = new VectorT; // amount of output signal in mixdown

			forwards->resize(order + 1);
			backs->resize(order);
			input->resize(2 * (order + 1), N); // allows for circular buffering without modulo
			outputs->resize(2 * (order + 1), N); // N circular buffers of output

			modulator->resize(N);
			demodulator->resize(N);
			
			phasor->resize(N);
			dephasor->resize(N);

			preamps_in->resize(N);
			preamps_out->resize(N);
			
			gains_in->resize(N);
			gains_out->resize(N);

			forwards->setZero();
			backs->setZero();
			input->setZero();
			outputs->setZero();

			modulator->setZero();
			demodulator->setZero();
			phasor->setOnes();
			dephasor->setOnes();
			
			preamps_in->setZero();
			preamps_out->setZero();

			gains_in->setZero();
			gains_out->setZero();
		}

		// initalize the filter coefficients
		void coefficients(const std::vector<T>& forward, const std::vector<T>& back)
		{
			int coeffs = std::min<int>(order + 1, forward.size());
			for (int i = 0; i < coeffs; i++)
				(*forwards)(i) = forward[i];

			coeffs = std::min<int>(order, back.size());
			for (int i = 0; i < coeffs; i++)
				(*backs)(i) = back[i];
		}

		// initialize nth modulation and demodulation frequency
		void frequency(int n, T mod, T demod)
		{
			(*modulator)(n) = mod;
			(*demodulator)(n) = demod;
		}

		// set nth preamp coefficient
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

		// set nth mixdown coefficient
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
		VectorT* forwards; // feedforward coeffs
		VectorT* backs; // feedback coeffs

		MatrixT* input;
		MatrixT* outputs;

		VectorT* preamps_in;
		VectorT* preamps_out;

		VectorT* gains_in;
		VectorT* gains_out;

		ArrayT* modulator;
		ArrayT* demodulator;
		ArrayT* phasor;
		ArrayT* dephasor;

		int N; // number of filters
		int order; // highest order of filters involved
		double smoothing_p, smoothing_g;

		int origin = 0;
		bool computed = false; // flag in case of repeated calls to operator()

		void compute(T sample)
		{
			*preamps_out = (1 - smoothing_p) * (*preamps_in) + smoothing_p * (*preamps_out);
			*gains_out = (1 - smoothing_g) * (*gains_in) + smoothing_g * (*gains_out);

			VectorT therow = (sample * (*phasor)).matrix();
			input->row(origin) = therow;
			input->row(origin + (order + 1)) = therow;

			VectorT temp = (((*forwards) * input->block(origin, 0, order + 1, N)).array() * preamps_out->array()).matrix()
						 - ((*backs) * outputs->block(origin + 1, 0, order, N)).diagonal();

			temp = (temp.array() * (*dephasor)).matrix();

			outputs->row(origin) = temp;
			outputs->row(origin + (order + 1)) = temp;

			*phasor *= *modulator;
			*dephasor *= *demodulator;

			computed = true;
		}
	};
}

#define NETWORK
#endif