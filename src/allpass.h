// allpass.h
#include "includes.h"

/*

an allpass filter is one with symmetric conjugated feedforward/back coefficients
the allpass filters we implement here will be inherently complex

any composition (series) of allpass filters is allpass, and furthermore
there is an "interleaving" operation that combines two allpass and yields an allpass:
on the level of Laplace transforms, it is composition. 


*/