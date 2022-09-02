// world.h

#include "includes.h"

#ifndef WORLD

using Eigen::Matrix;
using Eigen::Dynamic;
using Eigen::Map;
using Eigen::seqN;
using Eigen::Array;

namespace soundmath
{
	template <typename T> class Binding
	{

	};

	template<typename T> class Spring : public Binding<T>
	{

	};

	template<typename T> class Gravity : public Binding<T>
	{

	};

	template <typename T> class World
	{
		typedef Matrix<T, Dynamic, Dynamic> MatrixT;
		typedef Matrix<T, Dynamic, 1> VectorT;

	public:
		World(int N, T drag = 0.01) : N(N), drag(drag)
		{
			positions = new VectorT;
			masses = new VectorT;
			velocities = new VectorT;
			accelerations = new VectorT;

			positions->resize(N);
			masses->resize(N);
			velocities->resize(N);
			accelerations->resize(N);

			positions->setZero();
			masses->setOnes();
			velocities->setZero();
			accelerations->setZero();
		}

		~World()
		{

		}

		void tick()
		{
			(*velocities) += (*accelerations) / SR;
			(*velocities) *= drag;
			(*positions) += (*velocities) / SR;
		}

		VectorT* operator()()
		{
			return positions;
		}

	private:
		T drag;
		int N;

		VectorT* positions;
		VectorT* masses;
		VectorT* velocities;
		VectorT* accelerations;

		std::vector<Binding<T>*> bindings;
	};
}

#define WORLD