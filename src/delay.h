// delay.h
#include "includes.h"
#include "buffer.h"

#ifndef DELAY

namespace soundmath
{
	// template <typename T> class Delay
	// {
	// public:
	// 	Delay() { }
	// 	~Delay() { }

	// 	// a "sparse" filter
	// 	Delay(const std::vector<std::pair<uint, T>>& feedforward, const std::vector<std::pair<uint, T>>& feedback)
	// 	{
	// 		forward = feedforward;
	// 		back = feedback;

	// 		order = 0;
	// 		for (int i = 0; i < forward.size(); i++)
	// 			if (forward[i].first > order)
	// 				order = forward[i].first;

	// 		for (int i = 0; i < back.size(); i++)
	// 		{
	// 			if (back[i].first > order)
	// 				order = back[i].first;

	// 			if (back[i].first == 0)
	// 				back[i].second = 0;
	// 		}

	// 		order++; // make room in the ring buffer

	// 		reset();
	// 	}

	// 	void reset()
	// 	{
	// 		output = std::vector<T>(order, 0);
	// 		history = std::vector<T>(order, 0);
	// 		origin = 0;
	// 		computed = false;
	// 	}

	// 	void tick()
	// 	{
	// 		origin++;
	// 		origin %= order;
	// 		computed = false;
	// 	}

	// 	T operator()(T sample)
	// 	{
	// 		if (!computed)
	// 		{
	// 			history[origin] = sample;

	// 			T out = 0;
	// 			uint index, delay;
	// 			T attenuation;

	// 			for (int i = 0; i < forward.size(); i++)
	// 			{
	// 				delay = forward[i].first;
	// 				attenuation = forward[i].second;
	// 				index = (origin - delay + order) % order;
	// 				out += attenuation * history[index];
	// 			}

	// 			for (int i = 0; i < back.size(); i++)
	// 			{
	// 				delay = back[i].first;
	// 				attenuation = back[i].second;
	// 				index = (origin - delay + order) % order;
	// 				out -= attenuation * output[index];
	// 			}

	// 			output[origin] = out;
	// 			computed = true;
	// 		}

	// 		return output[origin];
	// 	}

	// private:
	// 	int order;
	// 	int origin;
	// 	bool computed;

	// 	std::vector<std::pair<uint, T>> forward;
	// 	std::vector<std::pair<uint, T>> back;
	// 	std::vector<T> output;
	// 	std::vector<T> history;
	// };

	// many delays
	template <typename T> class Delay
	{
	public:
		Delay() { }
		~Delay()
		{
			delete [] forwards;
			delete [] backs;
		}

		// initialize N delays of given sparsity and maximum time
		Delay(uint sparsity, uint time) : input(time + 1), output(time + 1)
		{
			forwards = new std::pair<uint, T>[sparsity]; 
			backs = new std::pair<uint, T>[sparsity];

			for (int i = 0; i < sparsity; i++)
			{
				forwards[i] = {0, 0};
				backs[i] = {0, 0};
			}

			this->sparsity = sparsity;
			computed = false;
		}

		// initalize coefficients
		void coefficients(const std::vector<std::pair<uint, T>>& forward, const std::vector<std::pair<uint, T>>& back)
		{
			uint order = std::min(sparsity, (uint)forward.size());
			for (int i = 0; i < order; i++)
				forwards[i] = forward[i];
			for (int i = order; i < sparsity; i++)
				forwards[i] = {0, 0}; // zero out trailing coefficients

			order = std::min(sparsity, (uint)back.size());
			for (int i = 0; i < std::min(sparsity, (uint)back.size()); i++)
			{
				if (back[i].first != 0) // don't allow zero-time feedback
					backs[i] = back[i];
				else
					backs[i] = {0, 0};
			}
			for (int i = order; i < sparsity; i++)
				backs[i] = {0, 0}; // zero out trailing coefficients
		}

		// modulate the feedforward coeffs of nth delay
		void modulate_forward(uint n, const std::pair<uint, T>& forward)
		{ forwards[n] = forward; }

		// modulate the feedback coeffs of nth delay
		void modulate_back(uint n, const std::pair<uint, T>& back)
		{
			if (back.first == 0)
				backs[n] = {0, 0};
			else
				backs[n] = back;
		}

		// get the result of filter applied to a sample
		T operator()(T sample)
		{
			if (!computed)
			{
				input.write(sample);
				output.write(0);

				for (int i = 0; i < sparsity; i++) // i is delay time
				{
					std::pair<uint, T> forward = forwards[i];
					std::pair<uint, T> back = backs[i];
					output.accum(forward.second * input(forward.first) - back.second * output(back.first));
				}

				computed = true;
			}

			return output(0);
		}

		// timestep
		void tick()
		{
			input.tick();
			output.tick();
			computed = false;
		}

	private:
		Buffer<T> input; // circular buffers of inputs and outputs
		Buffer<T> output; 
		uint sparsity;

		std::pair<uint, T>* forwards; // feedforward times and coefficients
		std::pair<uint, T>* backs; // feedback times and coefficients

		bool computed; // flag in case of repeated calls to operator()
	};
}

#define DELAY
#endif