import sys
import mido
import time
from time import sleep
import numpy as np
import harmony
from random import random

size = 8
n = 12
aftertouch = False

def mtof(midi):
	return 440 * 2 ** ((midi - 69.0) / n)

# methods for interacting with control surface
class Launchpad:
	static = True

	def globals(self):
		static = False

		keys = np.zeros((size, size), dtype = 'byte')
		for j in range(size):
			keys[:,j] = range(10 * j + 11, 10 * j + size + 11)

		keys_lookup = {keys[x,y] : (x,y) for x in range(size) for y in range(size)}
		
		top = np.zeros(size + 2, dtype = 'byte')
		top[:] = range(90, 100)
		left = np.zeros(size, dtype = 'byte')
		left[:] = range(10, 81, 10)
		right = np.zeros(size, dtype = 'byte')
		right[:] = range(19, 90, 10)
		bottom = np.zeros((2, size), dtype = 'byte')
		bottom[0] = range(1, 9)
		bottom[1] = range(101, 109)

		control = [*bottom[0], *bottom[1], *top, *left, *right]
		
		names = ['arm', 'mute', 'solo', 'volume', 'pan', 'sends', 'device', 'stop_clip', \
				 'track_1', 'track_2', 'track_3', 'track_4', 'track_5', 'track_6', 'track_7', 'track_8', \
				 'shift', 'left', 'right', 'session', 'note', 'chord', 'custom', 'sequencer', 'projects', 'logo', \
				 'record', 'play', 'fixed_length', 'quantize', 'duplicate', 'clear', 'down', 'up', \
				 'print_to_clip', 'micro_step', 'mutation', 'probability', 'velocity', 'pattern_settings', 'steps', 'patterns']
		
		control_lookup = {control[i] : names[i] for i in range(len(control))}
		control = {v : k for k, v in control_lookup.items()}

		Launchpad.keys = keys
		Launchpad.keys_lookup = keys_lookup
		Launchpad.control = control
		Launchpad.control_lookup = control_lookup

	# the ioport should be a mido port for reading messages and
	# sending messages (e.g. for note illumination)
	# the output should be a mido port for sending note messages (e.g. a synth)
	def __init__(self, ioport, outputs):
		if self.static: self.globals()

		self.ioport = ioport
		self.outputs = outputs
		self.reset()

	def reset(self):
		for note in range(2 ** 7):
			self.ioport.send(mido.Message('note_off', note = note, velocity = 0))

	def read(self, message):
		if message.is_cc():
			if message.control in self.control_lookup: # control messages
				button = self.control_lookup[message.control]
				on = message.value > 0
				return ('cc', button, on)
			elif message.control == 64:
				on = message.value > 0
				return ('cc', 'sustain', on)

		elif message.type in ['note_on', 'note_off'] and message.note in self.keys_lookup: # note messages
			note, velocity = message.note, message.velocity
			on = bool((message.type == 'note_on') and velocity)
			pad = self.keys_lookup[note]
			return ('note', pad, on, velocity)

		elif aftertouch and message.type in ['polytouch'] and message.note in self.keys_lookup: # aftertouch messages
			note, velocity = message.note, message.value
			pad = self.keys_lookup[note]
			return ('touch', pad, velocity > 0, velocity)

		return ('other', None, None, None)


	def write(self, pad, color):
		self.ioport.send(mido.Message('note_on', note = self.keys[pad], velocity = color))

	def button(self, control, color):
		self.ioport.send(mido.Message('note_on', note = self.control[control], velocity = color))

	def play(self, pitch, velocity):
		for output in self.outputs:
			output.send(mido.Message('note_on', note = pitch, velocity = velocity))

	def touch(self, pitch, velocity):
		for output in self.outputs:
			output.send(mido.Message('polytouch', note = pitch, value = velocity))


class Instrument:
	def __init__(self, ioport, outputs):

		self.L = Launchpad(ioport, outputs)
		self.running = True

		self.mode = {'keyboard' : Keyboard(self), 'chord' : Chord(self), 'velocity' : Velocity(self)}
		self.application = self.mode['keyboard']

		self.play = lambda pitch, velocity : self.L.play(pitch, self.mode['velocity'].curve[velocity])
		self.touch = lambda pitch, velocity : self.L.touch(pitch, self.mode['velocity'].curve[velocity])
		self.button = lambda control, color : self.L.button(control, color)
		self.reset = lambda : self.L.reset()

		self.switch('keyboard')

	def quit(self):
		self.running = False
		self.reset()

	def process(self, message):
		command = self.L.read(message)
		kind = command[0]
		command = command[1:]
		if command is None: return

		if len(command) == 2:
			button, on = command

			if on:
				if button == 'shift': self.quit()
				[self.switch(mode) for mode in self.mode if button == self.mode[mode].button]

			self.application.process(command)
			return

		elif len(command) == 3 and kind == 'note':
			pad, on, velocity = command
			self.application.process(command)

		elif len(command) == 3 and kind == 'touch':
			pad, on, velocity = command
			self.application.touch(command)

	def switch(self, appname):
		self.application.suspend()
		self.application = self.mode[appname]
		self.application.startup()

	def display(self):
		self.application.print()
		for x in range(size):
			for y in range(size):
				self.L.write((x,y), self.application.display[x,y])


class Application:
	def __init__(self, owner):
		self.owner = owner

		self.display = np.zeros((size, size), dtype = 'byte') # stores state of 8 x 8 grid
		self.button = None # the button that opens the application
		self.color = None # the color of the appication "icon"

		self.piano = np.array([-1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0]) # C : -1, white : 0, black : 1
		
		# [neutral, pressed, triggered, sympathized, sympathizing] x [white, black, C]
		self.keycolor = np.array([[119, 0, 96], [40, 41, 60], [88, 101, 94], [104, 112, 92], [6, 7, 5]], dtype = 'byte')

		self.bins = {name : i for i, name in enumerate( \
			['track_1', 'track_2', 'track_3', 'track_4', 'track_5', 'track_6', 'track_7', 'track_8', \
			 'arm', 'mute', 'solo', 'volume', 'pan', 'sends', 'device', 'stop_clip'])}

		self.neutral = 0
		self.pressed = 1
		self.triggered = 2
		self.sympathized = 3
		self.sympathizes = 4

		self.range = range(21, 109) # piano range: A0 to C8

	def process(self, command):
		pass

	def touch(self, command):
		pass

	def suspend(self):
		if self.button:
			self.owner.button(self.button, 0)

	def startup(self):
		if self.button:
			self.owner.button(self.button, self.color)
			self.print()

	def print(self):
		pass


class Keyboard(Application):
	def __init__(self, owner):
		super().__init__(owner)

		self.button = 'note'
		self.color = 40
		self.handedness = 'right' # 'right', 'left', 'ambi'

		# some options: 36, 5, 1: bass
		#  				    7, 1: cello
		# 					1, 2: harpejji	
		# 				    4, 3: tonnetz
		self.tune(36, 5, 1) # C1 in lower-left corner, fours in columns, semitones in rows
		
		self.sustains = {(x,y) : set() for x in range(size) for y in range(size)} # sustains[(x,y)] = {notes held by pad (x,y)}
		self.sustains[()] = set() # () is the damper pedal "pad" coordinates
		self.sustained = {note : set() for note in self.range} # sustained[note] = {pads (x,y) that hold down note}

		self.harmonizes = {(x,y) : set() for x in range(size) for y in range(size)} # same as above, but for chords
		self.harmonized = {note : set() for note in self.range}

		self.resonates = {note : set() for note in self.range}
		self.resonated = {note : set() for note in self.range}
		self.resonance = None

		self.footlatch = False
		self.buttonlatch = False
		self.pedal = False # sustain pedal
		self.sustaintoggle = False # button-toggle for sustain
		
		self.history = []
		self.historical = True
		self.window = 1
		self.voicings = harmony.voicings

		self.harmonizer = None
		self.harmony = None
		
		self.rootless = False
		self.walking = False # chord buttons rather than pads trigger harmony (raised an octave if true)
		self.chorale = True # chord qualities are determined procedurally unless other structure is imposed
		self.mono = True # one chord at a time
		self.rootless = False
		self.melodize = False # melody notes are chosen; chords are supplied underneath

	def suspend(self):
		super().suspend()

		for pad in (coordinate for coordinate in self.sustains if coordinate and self.sustains[coordinate]):
			self.release(pad)
			self.unchord(pad, displacement = n if self.walking else 0)

	def tune(self, origin, tuning, frets):
		self.origin = origin # pitch of lower-left corner
		self.tuning = tuning # column interval
		self.frets = frets # row interval
		self.breadth = (size - 1) * tuning + (size - 1) * frets
		self.remap()

	def remap(self):
		self.map = np.array([[self.origin + self.frets * x + self.tuning * y for y in range(size)] for x in range(size)])

		if self.handedness == 'ambi':
			self.map[:,:size // 2] = self.map[:,:size // 2][::-1]

		elif self.handedness == 'left':
			self.map = self.map[::-1]

	def transpose(self, step):
		if self.origin + step in self.range and self.origin + step + self.breadth in self.range:
			self.origin += step

		self.remap()

	def print(self):
		self.keystate = np.zeros((size, size), dtype = 'byte')
		indices = self.origin, self.origin + self.breadth + 1
		inscope = [bool(self.sustained[i]) for i in range(*indices)]
		inharm = [bool(self.harmonized[i]) for i in range(*indices)]
		inreson = [bool(self.resonated[i]) for i in range(*indices)]
		insymp = [bool(self.resonates[i]) for i in range(*indices)]

		for y in range(size):
			self.keystate[:,y][inscope[self.tuning * y : self.tuning * y + self.frets * size : self.frets]] = self.pressed
			self.keystate[:,y][inharm[self.tuning * y : self.tuning * y + self.frets * size : self.frets]] = self.triggered
			self.keystate[:,y][inreson[self.tuning * y : self.tuning * y + self.frets * size : self.frets]] = self.sympathized
			self.keystate[:,y][insymp[self.tuning * y : self.tuning * y + self.frets * size : self.frets]] = self.sympathizes
			
			if self.handedness == 'left' or (self.handedness == 'ambi' and y < size // 2):
				self.keystate[:,y] = self.keystate[:,y][::-1]

		self.display = self.keycolor[self.keystate, self.piano[self.map % n]]


	def process(self, command):
		if len(command) == 2: # cc button
			button, on = command

			if button == 'sustain': # received sustain pedal cc message
				self.footlatch = on

			elif button == 'record': # button for sustain pedal function
				self.buttonlatch = on

			elif on and button == 'play':
				self.sustaintoggle = not self.sustaintoggle # pressing the play button switches this mode on and off

				if self.sustaintoggle: # just turned on
					self.owner.button('play', 1) # illuminate the button
				else: # just turned off
					self.owner.button('play', 0) # unlight the button
					
					for note in (root for root in self.resonates if self.resonates[root]):
						self.unresonate(note)

					self.resonates = {note : set() for note in self.range} # remove all resonance
					self.resonated = {note : set() for note in self.range}
					
					self.harmonizer = None
					self.harmony = None
					for trigger in self.bins: # dim all appropriate buttons
						self.owner.button(trigger, 7 if self.owner.saves[self.bins[trigger]] else 0)

			if button in ['sustain', 'record', 'play']:
				self.pedal = self.footlatch or self.buttonlatch or self.sustaintoggle # any one of these is sufficient

				if self.pedal:
					self.owner.button('record', 1)
					for pad in (coordinate for coordinate in self.sustains if coordinate and self.sustains[coordinate]):
						for note in self.sustains[pad]: # any notes held by any pads are also held by pedal
							self.sustains[()].add(note)
							self.sustained[note].add(())

					for pad in (coordinate for coordinate in self.harmonizes if self.harmonizes[coordinate]):
						for note in self.harmonizes[pad]: # any notes harmonized by any pads are also held by pedal
							self.sustains[()].add(note)
							self.sustained[note].add(())

					for note in self.resonates: # any note resonated by any note gets held by pedal
						for pitch in self.resonates[note]:
							self.sustains[()].add(pitch)
							self.sustained[pitch].add(())
				else:
					self.release(())
					self.owner.button('record', 0)

			elif button == 'duplicate': # retrigger all active midi notes
				if on:
					self.owner.button('duplicate', 1) # light up the duplicate button
					for note in self.sustained:
						if self.sustained[note] or self.harmonized[note] or self.resonated[note]:
							self.owner.play(note, 64)
				else:
					self.owner.button('duplicate', 0) # turn off the duplicate button

			elif on and button == 'up': self.transpose(self.tuning)
			elif on and button == 'down': self.transpose(-self.tuning)
			elif on and button == 'left': self.transpose(-self.frets)
			elif on and button == 'right': self.transpose(self.frets)

			elif on and button in self.bins: # a chord button is pressed
				track = self.bins[button]
				old = self.harmonizer

				idle = old != track and self.owner.saves[track]
				self.harmonizer = track if idle else None
				self.harmony = self.owner.saves[track] if idle else None

				for trigger in self.bins: # dim all appropriate buttons
					self.owner.button(trigger, 7 if self.owner.saves[self.bins[trigger]] else 0)

				if self.harmony:
					self.owner.button(button, 5) # if appropriate, light up button

				if not self.sustaintoggle and self.harmony and self.walking:
					for pad in (coordinate for coordinate in self.sustains if coordinate and self.sustains[coordinate]):
						self.voicelead(pad)
						self.playchord(pad, 32, displacement = n)

			elif not on and button in self.bins: # a chord button is released
				track = self.bins[button]

				if not self.sustaintoggle:
					self.owner.button(button, 7 if self.owner.saves[track] else 0)

					if self.walking:
						for pad in (coordinate for coordinate in self.sustains if self.sustains[coordinate]):
							self.unchord(pad, displacement = n)
					
					if self.harmonizer == track: # no more chord buttons held down
						self.harmonizer = None
						self.harmony = None

		elif len(command) == 3: # one of the 8 x 8 pressure-sensitive pads
			pad, on, velocity = command

			if on: # button pressed
				self.sustains[pad].add(self.map[pad]) # this pad sustains the pitch self.map[pad]
				self.sustained[self.map[pad]].add(pad) # the pitch self.map[pad] is sustained by pad

				if self.pedal:
					if self.sustaintoggle:
						ringing = self.resonates[self.map[pad]] # things the pressed note button resonates
						if not ringing or self.harmonizer != self.resonance: # either no ringing or new chord button
							if not (self.harmonizer is None): # if a chord button
								roots = [root for root in self.resonates if self.resonates[root] and root != self.map[pad]]

								pitches = self.sympathize(self.map[pad])
								self.unresonate(self.map[pad], ringing - pitches)
								self.resonate(self.map[pad], pitches, 32)
								self.resonance = self.harmonizer

								if self.mono:
									for root in roots:
										self.unresonate(root)

							else:
								if self.resonated[self.map[pad]]:
									for note in self.resonated[self.map[pad]]: # for note holding down this note
										self.resonates[note] -= {self.map[pad]}
									self.resonated[self.map[pad]] = set()
									self.sustains[()] -= {self.map[pad]}
									self.sustained[self.map[pad]] -= {()}

								else:
									self.sustains[()] ^= {self.map[pad]}
									self.sustained[self.map[pad]] ^= {()}
						
						else:
							self.unresonate(self.map[pad])
							self.sustains[()] -= {self.map[pad]}
							self.sustained[self.map[pad]] -= {()}

					else:
						self.sustains[()].add(self.map[pad])
						self.sustained[self.map[pad]].add(())
			
				if not (self.harmony and self.rootless):
					self.owner.play(self.map[pad], velocity)

				if not self.sustaintoggle and self.harmony and not self.walking:
					self.voicelead(pad)	
					if self.mono:
						for old in (old for old in self.harmonizes if self.harmonizes[old]):
							self.release(old)
							self.unchord(old)
						self.playchord(pad, velocity = 32)
					else:
						self.playchord(pad, velocity = 32)

			else: # button released
				if self.sustains[pad]:
					self.release(pad)
					self.unchord(pad, displacement = n if self.walking else 0)

	def touch(self, command):
		pad, on, velocity = command

		if on:			
			if not (self.harmony and self.rootless):
				self.owner.touch(self.map[pad], velocity)

	def voicelead(self, pad):
		roots = self.owner.roots[self.harmonizer]
		roots = roots if roots else self.harmony
		condensed = {note % n for chord in self.history for note in chord}

		structures = []
		for root in roots: # find all upper structures	
			structure = [(note - root) % n for note in self.harmony if note != root]
			structures.append(structure)

		options = []
		for structure in structures:
			for voicing in self.voicings[len(structure)]:
				option = harmony.voice(structure, voicing) # this will be sorted
				if self.melodize:
					top = option[-1]
					option = (0,) + option[:-1]
					option = [self.map[pad] + (note - top) for note in option]
				else:
					option = [self.map[pad] + note for note in option]
				option = [note for note in option if note in self.range]
				options.append(option)

		last = None
		if self.history:
			last = self.history[-1]
			best = min(options, key = lambda x : self.distance(last, x) + \
												 self.historical * len(condensed ^ set(p % n for p in x)) ** 2)
		else:
			best = options[int(random() * len(options))]

		self.history.append(best)
		if len(self.history) > self.window:
			self.history.pop(0)

	def playchord(self, pad, velocity, displacement = 0):
		chord = set(pitch + displacement for pitch in self.history[-1])
		chord = set(pitch for pitch in chord if pitch in self.range)

		self.harmonizes[pad].update(chord)
		for pitch in chord:
			self.harmonized[pitch].add(pad)

		if self.pedal:
			self.sustains[()].update(chord)
			for pitch in chord:
				self.sustained[pitch].add(())

		for pitch in chord:
			self.owner.play(pitch, velocity)

	def sympathize(self, note):
		pitches = {}
		roots = self.owner.roots[self.harmonizer]
		if len(roots) == 1:
			root = list(roots)[0]
			structure = sorted([(pitch - root) % n for pitch in self.harmony])
			octaves = (self.range[-1] - note) // n + 1
			pitches = [note + p + i * n for p in structure for i in range(octaves)]
			pitches = [p for p in pitches if p in self.range]
			# frequencies = [mtof(p) for p in pitches]

		return set(pitches)

	def resonate(self, note, pitches, velocity):
		self.resonates[note].update(pitches)
		for pitch in pitches:
			self.sustains[()] -= {pitch}
			self.sustained[pitch] -= {()}
			self.resonated[pitch].add(note)

			self.owner.play(pitch, velocity)

	def unresonate(self, note, subset = None):
		subset = self.resonates[note] if subset is None else subset
		for pitch in subset: # subset describes the pitches to free
			self.resonated[pitch] -= {note}
			if not len(self.sustained[pitch] | self.harmonized[pitch] | self.resonated[pitch]):
				self.owner.play(pitch, 0)

		self.resonates[note] = set()

	def release(self, pad):
		for note in self.sustains[pad]:
			self.sustained[note] -= {pad}
			if not len(self.sustained[note] | self.harmonized[note] | self.resonated[note]):
				self.owner.play(note, 0)

		self.sustains[pad] = set()

	def unchord(self, pad, displacement = 0):
		for note in self.harmonizes[pad]:
			self.harmonized[note] -= {pad}
			if not len(self.sustained[note] | self.harmonized[note] | self.resonated[note]):
				self.owner.play(note, 0)

		self.harmonizes[pad] = set()

	def distance(self, class1, class2):
		if len(class1) < len(class2):
			return min(self.distance(class1[:i+1] + class1[i:], class2) for i in range(len(class1)))

		elif len(class1) > len(class2):
			return self.distance(class2, class1)

		distance = 0
		for i in range(len(class1)):
			distance += self.penalty(abs(class1[i] - class2[i]))
		
		return distance

	def penalty(self, gap):
		return gap ** 2


class Chord(Application):
	def __init__(self, owner):
		super().__init__(owner)

		self.button = 'chord'
		self.color = 88

		self.clock = [(6,4), (5,5), (4,6), (3,6), (2,5), (1,4), (1,3), (2,2), (3,1), (4,1), (5,2), (6,3)]
		self.keycolor[0,1] = 1

		# self.registers = [[i for i in self.range if i % n == k] for k in range(n)]
		self.registers = [[i] for i in range(48, 60)]
		
		self.owner.saves = [set() for i in range(2 * size)]
		self.owner.roots = [set() for i in range(2 * size)]
		self.recording = None

		self.rootmode = False
		self.corners = [(0,0), (7,0), (0,7), (7,7)]

		self.sustains = [self.neutral for i in range(len(self.clock))]

	def suspend(self):
		super().suspend()

		self.recording = None
		self.sustains = [self.neutral for i in range(len(self.clock))]
		self.unsound(range(n))

	def print(self):
		for i, pos in enumerate(self.clock):
			self.display[pos] = self.keycolor[self.sustains[i], self.piano[i]]

		if self.recording is not None:
			color = self.pressed if self.rootmode else self.triggered
			for corner in self.corners:
				self.display[corner] = self.keycolor[color, 1]

		else:
			for pad in self.corners: # + self.borders:
				self.display[pad] = 0

	def paint(self, track):
		for pitch in self.owner.saves[track]:
			self.sustains[pitch] = self.triggered

		for pitch in self.owner.roots[track]:
			self.sustains[pitch] = self.pressed

	def unpaint(self, pitches):
		for pitch in pitches:
			self.sustains[pitch] = self.neutral

	def sound(self, track, velocity = 64):
		for pitch in self.owner.saves[track]:
			self.shepard(pitch, 64)

	def unsound(self, pitches):
		for pitch in pitches:
			self.shepard(pitch, 0)

	def process(self, command):
		if len(command) == 2:
			button, on = command

			if on and button in self.bins: # a chord button is pressed
				track = self.bins[button]
				self.owner.button(button, 5)

				if self.recording is not None:
					pitches = self.owner.saves[self.recording]
					self.unsound(pitches)
					self.unpaint(pitches)

				self.sound(track)
				self.paint(track)
				self.recording = track

			elif not on and button in self.bins: # a chord button is released
				track = self.bins[button]
				self.owner.button(button, 7 if self.owner.saves[track] else 0)
				
				if self.recording == track: # no more chord buttons held down
					pitches = range(n)
					self.unsound(pitches)
					self.unpaint(pitches)
					self.recording = None
					self.rootmode = False

			elif on and self.recording is not None and button in ['left', 'right']: # a chord button is held, arrow key pressed
				displacement = 1 if button == 'left' else - 1
				newsaves = set((a + displacement) % n for a in self.owner.saves[self.recording])
				newroots = set((a + displacement) % n for a in self.owner.roots[self.recording])

				togo = self.owner.saves[self.recording] - newsaves
				self.unsound(togo)
				self.unpaint(togo)

				self.owner.saves[self.recording] = newsaves
				self.owner.roots[self.recording] = newroots

				self.sound(self.recording)
				self.paint(self.recording)

		elif len(command) == 3:
			pad, on, velocity = command

			if pad in self.clock:
				note = self.clock.index(pad)

				if self.recording is None: # no chord buttons pressed
					if on:
						self.sustains[note] = self.triggered
						self.shepard(note, 0)
						self.shepard(note, velocity)
					else:
						self.sustains[note] = self.neutral
						self.shepard(note, 0)

				else: # at least one chord button pressed
					if self.rootmode: # if in root-editing mode
						if on and note in self.owner.saves[self.recording]:
							self.owner.roots[self.recording] ^= {note}
							if self.sustains[note] == self.triggered:
								self.shepard(note, 0)
								self.shepard(note, velocity)
							else:
								self.sustains[note] = self.triggered
							self.paint(self.recording)

					elif on: # not in root-editing mode; some pad is pressed
						self.owner.saves[self.recording] ^= {note}
						self.owner.roots[self.recording] &= self.owner.saves[self.recording]

						if note not in self.owner.saves[self.recording]:
							self.sustains[note] = self.neutral
							self.shepard(note, 0)

						self.sound(self.recording)
						self.paint(self.recording)

			elif on and self.recording is not None:
				self.rootmode = not self.rootmode

	def shepard(self, note, velocity):
		k = len(self.registers[note])
		for i, pitch in enumerate(self.registers[note]):
			self.owner.play(pitch, int(velocity * ((i + 0.5) / k) * (1 - (i + 0.5) / k)))


class Velocity(Application):
	def __init__(self, owner):
		super().__init__(owner)

		self.button = 'custom'
		self.color = 48
		self.colors = [48, 49, 50, 51]

		self.tester = None
		self.testcolors = [8, 9, 10, 11]

		self.curve = None
		self.bars = None
		self.reset()

	def startup(self):
		super().startup()
		self.owner.button('clear', self.color)

	def suspend(self):
		super().suspend()
		self.owner.button('clear', 0)

	def reset(self):
		self.curve = np.array([i for i in range(2 ** 7)], dtype = 'byte')
		self.bars = np.array([i for i in range(size)], dtype = 'float')

	def fit(self):
		bars = np.zeros(len(self.bars) + 1, dtype = 'float')
		bars[:-1] = self.bars
		bars[-1] = 2 * bars[-2] - bars[-3]
		skip = 2 ** 7 // size
		for x in range(size):
			for t in range(skip):
				self.curve[x * skip + t] = min(2 ** 7 - 1, max(0, \
					int((1 - t / skip) * skip * bars[x] + (t / skip) * skip * bars[x+1])))

		self.curve[0] = 0

	def process(self, command):

		if len(command) == 2:
			button, on = command

			if on and button == 'clear':
				self.reset()

			if on and button == 'print_to_clip':
				print(self.curve)
				print(self.bars)

		elif len(command) == 3:
			pad, on, velocity = command

			if pad == (0,7):
				self.tester = velocity

			elif on:
				for x in range(size):
					if x == pad[0]:
						self.bars[x] = (self.bars[x] + pad[1]) / 2

				self.fit()

	def print(self):
		self.display[:,:] = 0
		self.display[0,-1] = 9

		for x in range(size):
			bar = int(self.bars[x])
			top = self.colors[-int(len(self.colors) * (self.bars[x] - bar)) - 1]
			self.display[x, 0 : bar] = self.colors[0]
			self.display[x, bar] = top


		if self.tester:
			index = int(self.tester * (size / 2 ** 7) + 1/2)
			jndex = int(self.curve[self.tester] * (size / 2 ** 7) + 1/2)
			self.display[0 : index, 0] = 9
			self.display[index - 1, 0 : jndex] = 9


ports = mido.get_ioport_names()
sustainer = 'IAC Driver Bus 2'
surface = 'Launchpad Pro MK3 LPProMK3 MIDI'
assert surface in ports

synth = 'IAC Driver Bus 1'
multiout = False

if len(sys.argv) > 1:
	synth = sys.argv[1]
	multiout = True

print('in: ' + str(surface) + ', out: ' + str(synth))

programmer = mido.Message('sysex', data = [0, 32, 41, 2, 14, 14, 1])
ableton = mido.Message('sysex', data = [0, 32, 41, 2, 14, 14, 0])

with mido.open_ioport(surface) as ioport, \
	 mido.open_output(synth) as output, \
	 mido.open_input(sustainer) as pedal:
	try:
		if multiout:
			extra = mido.open_output(None)

		outputs = [output, extra] if multiout else [output]
		inst = Instrument(ioport, outputs)
		ioport.send(programmer)
		inst.display()

		while inst.running:
			for message in pedal.iter_pending():
				inst.process(message)
				inst.display()

			for message in ioport.iter_pending():
				inst.process(message)
				inst.display()

	except KeyboardInterrupt:
		pass

	inst.reset()
	# ioport.send(ableton)
	

'''
 - draw your own velocity curve
 - chord / voicing mode -- store pitch-class sets in banks (maybe also provide substitutions)
 	- rootless mode (roots do not sound)
 	- clockface mode (draw clock logo for chord storage)
 	- stored chords may be endowed list of permissible roots
 - 
'''
