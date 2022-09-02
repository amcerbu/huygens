#include "../src/rmsbank.h"
#include "../src/oscbank.h"
#include "../src/latchbank.h"
#include "../src/mixer.h"
#include "../src/signal.h"

using namespace std;
using namespace soundmath;

#define N 1

RMSbank<double, N> rms;
Oscbank<double, N> oscbank;
Mixer<double, N> mixdown;
Latchbank<double, N> latchbank(0.9, 0.2);
Signal<double, N> s;

int main()
{
	for (int i = 0; i < SR / 10; i++)
	{
		double the_sample = mixdown(latchbank((s({abs((double)(i % 100) / 50 - 1)}))));

		if (i % 1 == 0)
			cout << the_sample << endl;

		rms.tick();
		oscbank.tick();
	}
	return 0;
}