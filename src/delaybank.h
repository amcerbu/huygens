// delaybank.h
#include "includes.h"
#include "multichannel.h"

using Eigen::Matrix;
using Eigen::Map;
using Eigen::Array;

#ifndef DELAYBANK

namespace soundmath
{
	// bank of stickers -- nonlinear low-pass filters
	// N ins, N outs
	template <typename T, int N> class Delaybank : public Multichannel<T, N>
	{
	private:
		using typename Multichannel<T, N>::MatrixCT;
		using typename Multichannel<T, N>::VectorCT;
		using typename Multichannel<T, N>::ArrayCT;
		using typename Multichannel<T, N>::ArrayT;
		using Multichannel<T, N>::active;
		using Multichannel<T, N>::where;

	public:
		using Multichannel<T, N>::activate;
		using Multichannel<T, N>::deactivate;
		using Multichannel<T, N>::open;
		using Multichannel<T, N>::close;
		using Multichannel<T, N>::activity;

	public:
		Delaybank() : width(width)
		{
			averager = new Delaybank<T, N>(2, width);
			averager->coefficients({{0, 1}, {width, -1}}, {{1,-1}});
		}

		~Delaybank()
		{
			delete averager;
			delete out;
		}

		const ArrayCT* operator()(const ArrayCT* input)
		{
			*out = ((*averager)(input->abs2()) / width).sqrt();
			return out;
		}

	private:
		int width;

		Delaybank<T, N>* averager;
		ArrayT* out;

	};
}

#define DELAYBANK
#endif