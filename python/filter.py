import numpy as np
from mpl_toolkits.mplot3d import Axes3D 
import matplotlib.pyplot as plt
from matplotlib import cm

from mayavi import mlab

from importlib import reload


# add polynomials (P[0] + P[1]t + ...) + (Q[0] + Q[1]t + ...)
def plus(P, Q):
	deg = max(len(P), len(Q))
	return [(P + [0] * (deg - len(P)))[i] + (Q + [0] * (deg - len(Q)))[i] for i in range(deg)]

# multiply polynomials (P[0] + P[1]t + ...)(Q[0] + Q[1]t + ...)
def mult(P, Q):
	if len(P) > len(Q):
		return mult(Q, P)

	if len(P) == 1:
		return [P[0] * q for q in Q]

	split = len(P) // 2
	return plus(mult(P[:split], Q), [0] * split + mult(P[split:], Q))

# returns coefficients of a polynomial with given zeros
def coefficients(zeros):
	if len(zeros) == 0:
		return [1]
	elif len(zeros) == 1:
		return [-zeros[0], 1]

	split = len(zeros) // 2
	return mult(coefficients(zeros[:split]), coefficients(zeros[split:]))


class Filter:
	def __init__(self, forward, back):
		self.forward = forward;
		self.back = back;
		self.back[0] = 0;
		self.order = max(len(self.forward), len(self.back));

		self.forward += [0 for i in range(self.order - len(self.forward))];
		self.back += [0 for i in range(self.order - len(self.back))];
		self.order += 1;

		self.forget()

	def forget(self):
		self.output = [0 for i in range(self.order)];
		self.history = [0 for i in range(self.order)];
		self.origin = 0;
		self.computed = False

	def tick(self):
		self.origin += 1;
		self.origin %= self.order;
		self.computed = False;

	# returns the filter's z-transform applied to a point in the complex plane
	def z(self, frequency):
		return sum(a * (frequency ** n) for (n, a) in enumerate(self.forward)) / \
			   sum(a * (frequency ** n) for (n, a) in enumerate([1] + self.back[1:]))

	def __call__(self, sample):
		if not self.computed:
			self.history[self.origin] = sample;

			out = 0;
			for i in range(self.order - 1):
				index = (self.origin - i + self.order) % self.order;
				out += self.forward[i] * self.history[index] - self.back[i] * self.output[index];

			self.output[self.origin] = out;
			self.computed = True;

		return self.output[self.origin]

	def __add__(self, other):
		F = Filter(plus(mult(self.forward, [1] + other.back[1:]), mult(other.forward, [1] + self.back[1:])), \
						mult([1] + self.back[1:], [1] + other.back[1:]))
		return F

	def __sub__(self, other):
		return self + (-1 * other)

	def __rmul__(self, other):
		if type(other) is Filter:
			return self.__mul__(other)

		F = Filter([a * other for a in self.forward], self.back[:])
		return F

	def __mul__(self, other):
		F = Filter(mult(self.forward, other.forward), mult([1] + self.back[1:], [1] + other.back[1:]))
		return F


	# these functions are diagnostic and should not be used online

	# print out an impulse response
	def impulse(self, N = 100):
		self.forget()
		response = np.zeros(N);
		for i in range(N):
			self.tick()
			response[i] = self((int)(not i))

		return response

	# measures (empirical) frequency and phase response of DC, plus M harmonics up to Nyquist, using N samples
	# as N -> infinity, the last column of the returned matrix converges to the Fourier transform of self.impulse(M)
	def response(self, M, N):
		omega = np.pi * 1j
		responses = np.zeros((M + 1, N), dtype = 'complex')
		for i in range(M + 1):
			self.forget()
			for j in range(N):
				zeta = np.exp(j * (i / M) * omega)
				responses[i,j] = self(zeta) / zeta
				self.tick()

		return responses

	# measures (true) frequency and phase response of DC, plus M harmonics up to Nyquist; Fourier transform of self.impulse(M)
	def true_response(self, M):
		return np.array([self.z(np.exp(-1j * np.pi * i / M)) for i in range(M + 1)])

	# identical to response, except center frequencies are logarithmically spaced
	def perceptual(self, M, N, spread = 12):
		omega = np.pi * 1j
		responses = np.zeros((M + 1, N), dtype = 'complex')
		for i in range(M + 1):
			self.forget()
			for j in range(N):
				zeta = np.exp(j * (i > 0) * 2 ** (spread * ((i / M) - 1)) * omega)
				responses[i,j] = self(zeta) / zeta
				self.tick()

		return responses		

	# samples Fourier transform of impulse at logarithmically spaced frequencies
	def true_perceptual(self, M, spread = 12):
		return np.array([self.z(np.exp(-1j * np.pi * (i > 0) * 2 ** (spread * ((i / M) - 1)))) for i in range(M + 1)])

	def eq(self, M, spread = 12):
		A = self.true_perceptual(M, spread)
		plt.plot(np.abs(A)); plt.plot(np.real(A)); plt.plot(np.imag(A)); plt.show()

	# identical to response, but provides raw output of filter
	def pushthrough(self, M, N):
		omega = np.pi * 1j / M
		responses = np.zeros((M + 1, N), dtype = 'complex')
		for i in range(M + 1):
			self.forget()
			for j in range(N):
				zeta = np.exp(j * i * omega)
				responses[i,j] = self(zeta)
				self.tick()

		return responses

	# plots logarithmic
	def plot(self):
		pass


# biquad filter with poles of radius rad (< 1) at +-angle (in (0, pi))
def bandpass(angle, rad):
	# F = Filter(coefficients([1,-1]), coefficients([rad * np.exp(angle * 1j), rad * np.exp(-angle * 1j)]))
	F = Filter([1,0,-1], [1, -2 * np.cos(angle) * rad, rad * rad])
	normalization = np.abs(F.z(np.exp(angle * 1j)))
	return (1 / normalization) * F

def resonant(angle, rad):
	# F = Filter(coefficients([1,-1]), coefficients([rad * np.exp(angle * 1j), rad * np.exp(-angle * 1j)]))
	F = Filter([1], [1, -2 * np.cos(angle) * rad, rad * rad])
	normalization = np.abs(F.z(np.exp(angle * 1j)))
	return (1 / normalization) * F

def decay(angle, rad, N = 2048):
	F = badnpass(angle, rad)
	A = F.impulse(N)[[int(2 * i * np.pi / angle) for i in range(1 + int(N * angle / (2 * np.pi)))]]
	return A[2:] / A[1:-1]

def mtof(midi):
	return 440 * 2 ** ((midi - 69) / 12)

def ftoa(frequency, SR = 48000):
	return 2 * np.pi * frequency / SR

def mtoa(midi, SR = 48000):
	return ftoa(mtof(midi), SR)


def mpl_surface(A):
	fig, ax = plt.subplots(subplot_kw={"projection": "3d"})

	x = np.arange(0, A.shape[1])
	y = np.arange(0, A.shape[0])
	X, Y = np.meshgrid(x, y)

	ax.plot_surface(X, Y, np.real(A), cmap = cm.viridis, alpha = 0.9, antialiased = False, rstride = 4, cstride = 4)
	ax.plot_surface(X, Y, np.imag(A), cmap = cm.cividis, alpha = 0.9, antialiased = False, rstride = 4, cstride = 4)
	# ax.plot_surface(X, Y, np.abs(A), cmap = cm.plasma, alpha = 0.8, antialiased = False, rstride = 4, cstride = 4)

	plt.show()

def surface(A):
	fig = mlab.figure()
	surf1 = mlab.surf(np.abs(A), warp_scale = 'auto', colormap = 'Dark2')

	mlab.show()

Id = Filter([1], [0])
X = [Id - (0.9 ** i) * bandpass(ftoa(440 * (i + 1)), 0.99) for i in range(7)]
G = Id - X[0] * X[1] * X[2] * X[3] * X[4]


def softclip(sample, width = 0.5):
	if abs(sample) < width:
		return sample

	sgn = int(0 < sample) - int(0 > sample)
	gap = sample - sgn * width;
	return sgn * width + (1 - width) * 2 / np.pi * np.math.atan(np.pi * gap / (2 * (1 - width)))
