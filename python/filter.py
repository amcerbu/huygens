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
