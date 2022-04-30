import numpy as np


class Stream:
	def __init__(self, order, dtype = 'double'):
		self.order = order
		self.dtype = dtype
		self.data = np.zeros(order).astype(dtype)

	def push(self, sample):
		self.data = np.concatenate(((sample,), self.data[1:]))


class Filter:
	def __init__(self):
		pass