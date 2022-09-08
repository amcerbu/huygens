scoring = False

import numpy as np
from random import random
from math import sqrt
import math
if scoring:
    import abjad

n = 12

# voice-leading distance
cacheall = False

# needs p and q sorted!
def distance(p, q, cache = {} if cacheall else None, islice = None, jslice = None):
    if islice is None: islice = (0, len(p))
    if jslice is None: jslice = (0, len(q))
    if cache is None: cache = {}
    
    x = p[slice(*islice)]
    y = q[slice(*jslice)]
    
    if len(x) == 1 or len(y) == 1:
        try: return cache[p, q, islice, jslice] 
        except:
            try:
                weight = (sum(x) * sum(y) / (len(x) * len(y)))
                # weight = 1
                penalty = weight * sum((a - b) ** 2 for a in x for b in y)
                cache[p, q, islice, jslice] = penalty
                return penalty
            except:
                print(x,y)
        
    options = (((((islice[0], islice[0] + i), (jslice[0], jslice[0] + j)),
                 ((islice[0] + i, islice[1]), (jslice[0] + j, jslice[1]))))
                   for i in range(1, islice[1] - islice[0]) for j in range(1, jslice[1] - jslice[0]))
    
    value = math.inf
    for A, B in options:
        try: left = cache[(p, q, *A)]
        except:
            left = distance(p, q, cache, *A)
            cache[(p, q, *A)] = left
            
        try: right = cache[B]
        except:
            right = distance(p, q, cache, *B)
            cache[(p, q, *B)] = right
            
        value = min(value, left + right)
        try: value = cache[p, q, islice, jslice] = min(value, cache[p, q, islice, jslice])
        except: cache[p, q, islice, jslice] = value
            
    return value

# tetrachords
def ball(a = (0,3,7,10), radius = 4):
    neighbors = [[] for i in range(radius + 1)]

    for i in range(n):
        for j in range(i + 1, i + n):
            for k in range(j + 1, j + n):
                for l in range(k + 1, k + n):
                    dist = distance(a, (i, j, k, l))
                    if dist <= radius:
                        neighbors[dist].append((i, j, k, l))

    columns = [2 + max(len(str(n)) for n in neighbors[i]) for i in range(radius + 1)]

    for i in range(-1, max(len(n) for n in neighbors)):
        output = ''
        for j in range(radius + 1):
            if i < 0:
                entry = f'd = {j}'
                pad = (columns[j] - len(entry))
                output += ' ' * (pad // 2 - 1) + entry + ' ' * (pad - pad // 2 + 1)
            elif i < len(neighbors[j]):
                entry = str(neighbors[j][i])
                output += entry + ' ' * (columns[j] - len(entry))
            else:
                output += ' ' * columns[j]

        print(output)

Bs = C = 0
Cs = Df = 1
D = 2
Ds = Ef = 3
E = Ff = 4
Es = F = 5
Fs = Gf = 6
G = 7
Gs = Af = 8
A = 9
As = Bf = 10
B = Cf = 11

tomidi = {'bs': 0, 'c': 0, 'cs': 1, 'df': 1, 'd': 2, 'ds': 3, 'ef': 3, 'e': 4, 'ff': 4, 'es': 5, 'f': 5,
          'fs': 6, 'gf': 6, 'g': 7, 'gs': 8, 'af': 8, 'a': 9, 'as': 10, 'bf': 10, 'b': 11, 'cf': 11}

numbers = {'c' : 0, 'd' : 2, 'e' : 4, 'f' : 5, 'g' : 7, 'a' : 9, 'b' : 11}
naturals = {v : k for k, v in numbers.items()}
accidentals = {'f' : -1, 's' : 1}

depth = 1
names = {p : ({naturals[p]} if p in naturals else set()) | 
                set(a + b * c for a in numbers for b in accidentals for c in range(1, depth + 1)
                    if 0 == (numbers[a] + accidentals[b] * c - p) % n) for p in range(n)}

# diatonic[i] is the spelling of i major
diatonic = [['c', 'd', 'e', 'f', 'g', 'a', 'b'], 
            ['df', 'ef', 'f', 'gf', 'af', 'bf', 'c'],
            ['d', 'e', 'fs', 'g', 'a', 'b', 'cs'],
            ['ef', 'f', 'g', 'af', 'bf', 'c', 'd'],
            ['e', 'fs', 'gs', 'a', 'b', 'cs', 'ds'],
            ['f', 'g', 'a', 'bf', 'c', 'd', 'e'],
            ['gf', 'af', 'bf', 'cf', 'df', 'ef', 'f'],
            ['g', 'a', 'b', 'c', 'd', 'e', 'fs'],
            ['af', 'bf', 'c', 'df', 'ef', 'f', 'g'],
            ['a', 'b', 'cs', 'd', 'e', 'fs', 'gs'],
            ['bf', 'c', 'd', 'ef', 'f', 'g', 'a'],
            ['b', 'cs', 'ds', 'e', 'fs', 'gs', 'as']]



# input: set of midi pitch values. output: lilypond chord notation
# transp parameter provides quick way to change octaves
# depth parameter controls max # of repeated sharps or flats
def spell(pitches, transp = 0, depth = 1): 
    def weight(k):
        concat = ''.join((c[1][-1] if len(c[1]) > 1 else '' for c in k))
        return concat.count('f') + concat.count('s')
    
    registered = tuple(((p + transp) % n, (p + transp) // n) for p in pitches)
    named = tuple((pitches[i], a, (r - 1 if a == 'bs' else r + 1 if a == 'cf' else r))
                      for i, (q, r) in enumerate(registered) for a in names[q])
    assignment = {p : None for p in pitches}

    remaining = set(pitches)
    while remaining:
        candidates = {}
        biggest = 0
        for root, key in enumerate(diatonic):
            intersection = tuple(sorted([a for a in named if a[1] in key]))
            if len(intersection) > biggest:
                biggest = len(intersection)
            if intersection in candidates:
                candidates[intersection] += 1
            else:
                candidates[intersection] = 1
                
        candidates = {k : v for k, v in candidates.items() if len(k) == biggest}
        lightness = min(weight(k) for k in candidates)
        candidates = {k : v for k, v in candidates.items() if weight(k) == lightness}
        
        cut = max(candidates, key = candidates.get)
        
        remaining -= set(c[0] for c in cut)
        named = tuple(a for a in named if a[0] in remaining)
        for c in cut:
            assignment[c[0]] = c[1:]
            
    return [a + (',' if b < 0 else '\'') * b for k, (a, b) in assignment.items()]

class Hierarchy:
    def __init__(self, terms):
        if type(terms) is int:
            self.atom = True
            self.data = terms
        
        elif type(terms) is Hierarchy:
            self.atom = terms.atom
            self.data = terms.data
        
        else: # is an iterable, we suppose
            self.atom = False
            self.data = [Hierarchy(t) for t in terms]
                    
    def __repr__(self):
        return f'Hierarchy({self.raw()})'

    def raw(self):
        if self.atom: return self.data
        return tuple(t.raw() for t in self)
    
    def copy(self):
        return Hierarchy(self.raw())
    
    def __add__(self, other):
        if self.atom:
            return Hierarchy(self.data + other)
        
        return Hierarchy(t + other for t in self)
    
    def __iadd__(self, other):
        if self.atom:
            self.data += other
            return self
            
        for t in self:
            t += other
        
        return self
    
    def __radd__(self, other): return self.__add__(other)
    
    def __sub__(self, other): return self.__add__(-other)

    def __truediv__(self, other):
        return Hierarchy((Hierarchy(other).raw(), self.raw()))

    def __getitem__(self, key):
        if type(key) is slice: return Hierarchy(self.data[key]) # allow slicing
        elif type(key) is tuple: return Hierarchy(self[k] for k in key) # allow permutations: self[(s1,s2,...)]
        
        # return atomic data
        if self.atom: return self.data
        return self.data[key]
    
    def __setitem__(self, key, value):
        self.data[key] = Hierarchy(value)
        
    def __len__(self):
        if self.atom: return 1
        return len(self.data)
        
    def height(self):
        if self.atom: return self.data
        return min((t.height() for t in self), default = math.inf)
    
    def ordered(self):
        if self.atom or len(self) == 0:
            return True
        
        return self[:1].height() <= self[1:].height() and self[0].ordered() and self[1:].ordered()
    
    def flatten(self):
        if self.atom: return (self.data,)
        return sum((a.flatten() for a in self), start = ())
    
    # returns a hierarchy describing the shape of self
    def shape(self):
        other = self.copy()
        other.arrange(1)
        other.ground()
        return other.raw()
    
    # adjust sub-hierarchies by octaves until self.ordered()
    def arrange(self, octave = n):
        if self.atom:
            return
        
        for i in range(len(self)):
            self[i].arrange(octave)
            head = self[i - 1].height()
            tail = self[i].height()
            discrep = head - tail if i else -octave
            self[i:] += octave * (1 + discrep // octave)
            
        return self
                    
    # finds first atomic element
    def first(self):
        if self.atom:
            return self.data
        elif len(self) == 0:
            return 0
        
        return self[0].first()
    
    def ground(self):
        adjustment = self.first()
        self += -adjustment
        return self
    
    # recursive grounding
    def center(self):
        if self.atom or len(self) == 0:
            return 
        
        adjustment = self.first()
        self += -adjustment

        for t in self:
            t.center()
        return self

            
H = Hierarchy((0, 7, (14, 15, (18, 5)), 22))
H

def permutations(pieces):
    if len(pieces) <= 1:
        return (pieces,)
    
    return tuple(q + (a,) for i, a in enumerate(pieces)
                            for q in permutations(pieces[:i] + pieces[i + 1:])[::-1])[::-1]

'''
() -> ()
(1,) -> ((1,))
(1,2) -> ((1,2),(2,1))
(1,2,3) -> ((1,2,3),(1,3,2),(2,1,3),(2,3,1),(3,1,2),(3,2,1))
(1,(2,3)) -> ((1,(2,3)),(1,(3,2)),((2,3),1),((3,2),1))
((1,2,3)) -> (((1,2,3)), ((1,3,2)), ((2,1,3)), ...)

'''
def reshapements(shape, skip = -1):
    if type(shape) is int:
        return (shape,)
    elif len(shape) == 0:
        return ()
    elif len(shape) == 1:
        return tuple((r,) for r in reshapements(shape[0]))

    return tuple(q + (b,) for i, a in enumerate(shape)
                            for q in reshapements(shape[:i] + shape[i + 1:], skip)[::skip]
                            for b in reshapements(a, skip)[::skip])[::skip]

# Let's approach our chords as hierarchies:
# Notice that this allows us to consider a distinguished "root"

# major = Hierarchy((0,(2,4,7,9,11)))
# major = Hierarchy((0,(2,(4,9),7,11)))
major = Hierarchy((0,(2,4,7,11)))
# minor = Hierarchy((0,((2,3),7,10))) # disallow minor 3 above natural 9 unless consecutive
minor = Hierarchy((0,((2,3),5,7,10))) # disallow minor 3 above natural 9 unless consecutive
diminished = Hierarchy((0,3,6,9))
halfdim = Hierarchy((0,((2,3),6,10))) # disallow minor 3 above natural 9 unless consecutive
# halfdim = Hierarchy((0,(3,6,10))) # disallow minor 3 above natural 9 unless consecutive
alts9 = Hierarchy((0,((3,4),8,10))) # disallow sharp 9 below major 3 unless consecutive
altf9 = Hierarchy((0,(4,8,10,13)))
domf9 = Hierarchy((0,(1,4,7,(9,10)))) # disallow natural 6 below dom 7 unless consecutive
doms5 = Hierarchy((0,(2,4,8,10))) # disallow natural 6 below dom 7 unless consecutive
domf5 = Hierarchy((0,(2,4,6,10))) # disallow natural 6 below dom 7 unless consecutive
dominant = Hierarchy((0,(2,4,(7,9,10)))) # disallow natural 6 below dom 7 unless consecutive
maj7s5f9 = Hierarchy((0,(1,4,8,11)))
sus = Hierarchy((0,(2,5,7,10)))

# library of chords
library = {
    'M'     : Hierarchy((4,7)),
    'M7'    : Hierarchy((4,7,11)),
    'M9'    : Hierarchy((2,4,7)),
    'M79'   : Hierarchy((2,4,7,11)),
    'M7913' : Hierarchy((2,4,7,(10,11))),
    'M79s11': Hierarchy((2,4,(6,7),11)),
    
    'm'     : Hierarchy((3,7)),
    'm7'    : Hierarchy((3,7,10)),
    'm9'    : Hierarchy(((2,3),7)),
    'm79'   : Hierarchy(((2,3),7,10)),
    'm7911' : Hierarchy(((2,3),5,7,10)),

    'D7'    : Hierarchy((4,7,10)),
    'D79'   : Hierarchy((2,4,7,10)),
    'D7f9'  : Hierarchy((1,4,7,10)),
    'D7f913': Hierarchy((1,4,7,(9,10))),
    'D7s5'  : Hierarchy(((2,4),8,10)),
    'D7f5'  : Hierarchy((2,4,6,10)),

    'A7s9'  : Hierarchy(((3,4),8,10)),
    'A7f9'  : Hierarchy((1,4,8,10)),

    'o7'    : Hierarchy((3,6,9)),
    'm7f5'  : Hierarchy((3,6,10)),
    'm79f5' : Hierarchy(((2,3),6,10)),

    'sus4'  : Hierarchy((2,5,(9,10))),
}

a = '''
| (Bf M79) | - (F D7s5)      | (F m7)    | (Bf D79) |
| (Ef M79) | -               | (Af D7f5) | -        |
| (Bf M79) | - (D m7)        | (C D79)   | -        |
| (C m7)   | (D m7f5) (G D7) | (C m7)    | (F D7s5) |
| (Bf M79) | - (F D7s5)      | (F m7)    | (Bf D79) |
| (Ef M79) | -               | (Af D7f5) | -        |
| (Bf M79) | - (D m7)        | (C D79)   | -        |
| (C m7)   | (F D7)          | (Bf M79)  | -        |
| (Cs m7)  | (Fs D79)        | (Bf M79)  | -        |
| (B m7)   | (E D79)         | (A M79)   | -        |
| (A m7)   | (D D79)         | (G M79)   | -        |
| (G m7)   | (C D79)         | (C m79)   | (F D7s5) |
| (Bf M79) | - (F D7s5)      | (F m7)    | (Bf D79) |
| (Ef M79) | -               | (Af D7f5) | -        |
| (Bf M79) | - (D m7)        | (C D79)   | -        |
| (C m7)   | (F D7)          | (Bf M79)  | -        |'''


class Chart:
    def __init__(self, chords):
        self.chords = chords

    def parse(self):
        pass

    def raw(self):
        pass

def realbook(tune):
    if tune == 'giant steps':
        # giant steps
        chart = [(B, major), (D, dominant), (G, major), (Bf, dominant), (Ef, major), (A, minor), (D, dominant),
                 (G, major), (Bf, dominant), (Ef, major), (Fs, dominant), (B, major), (F, minor), (Bf, dominant),
                 (Ef, major), (A, minor), (D, dominant), (G, major), (Cs, minor), (Fs, dominant), (B, major),
                 (F, minor), (Bf, dominant), (Ef, major), (Df, minor), (Gf, dominant)]

        time = [4, 4, 4, 4, 2, 4, 4,
                4, 4, 4, 4, 2, 4, 4,
                2, 4, 4, 2, 4, 4, 2, 4, 4, 2, 4, 4]

    elif tune == 'cyclic episode':
        # cyclic episode
        chart = [(Bf, minor), (Df, minor), (E, minor), (G, minor),
                 (C, minor), (D, alts9), (G, minor), (A, alts9),
                 (D, minor), (B, minor), (Af, minor), (F, minor),
                 (C, major), (C, minor), (Ef, minor), (Gf, minor)]

        time = [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]

    elif tune == 'there will never be another you':
        # there will never be another you
        chart = [(Ef, major), (D, halfdim), (G, domf9),
                 (C, minor), (Bf, minor), (Ef, domf9),
                 (Af, major), (Df, dominant), (Ef, major), (C, minor),
                 (F, dominant), (F, minor), (Bf, domf9),
                 (Ef, major), (D, halfdim), (G, domf9),
                 (C, minor), (Bf, minor), (Ef, dominant),
                 (Af, major), (Df, dominant), (Ef, major), (A, minor), (D, dominant),
                 (Ef, major), (D, domf9), (G, minor), (C, dominant), (F, minor), (Bf, dominant), (Ef, major)]

        time = [1, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2]


    elif tune == 'cherokee':


        chart = [(Bf, major), (F, doms5), (F, minor), (Bf, domf9),
                 (Ef, major), (Af, domf5),
                 (Bf, major), (G, minor), (C, dominant),
                 (C, minor), (D, library['m7f5']/0), (G, alts9), (C, minor), (F, doms5),
                 (Bf, major), (F, doms5), (F, minor), (Bf, domf9),
                 (Ef, major), (Af, domf5),
                 (Bf, major), (G, minor), (C, dominant),
                 (C, minor), (F, dominant), (Bf, major),
                 (Cs, minor), (Fs, dominant), (B, major),
                 (B, minor), (E, dominant), (A, major),
                 (A, minor), (D, dominant), (G, major),
                 (C, minor), (C, dominant), (C, minor), (F, doms5),
                 (Bf, major), (F, doms5), (F, minor), (Bf, domf9),
                 (Ef, major), (Af, domf5),
                 (Bf, major), (G, minor), (C, dominant),
                 (C, minor), (F, dominant), (Bf, major)]

        time = [2/3, 2, 1, 1, 1/2, 1/2, 2/3, 2, 1/2, 1, 2, 2, 1, 1,
                2/3, 2, 1, 1, 1/2, 1/2, 2/3, 2, 1/2, 1, 1, 1/2,
                1, 1, 1/2, 1, 1, 1/2, 1, 1, 1/2, 1, 1, 1, 1,
                2/3, 2, 1, 1, 1/2, 1/2, 2/3, 2, 1/2, 1, 1, 1/2]

    return chart, time

if scoring:
    def score(X):
        chords = [abjad.Chord(x[1]) for i, x in enumerate(X)]
        clef = abjad.Clef("bass")
        roots = [abjad.Note(x[0]) for i, x in enumerate(X)]
        for note in roots:
            abjad.attach(clef, note)
        staff1 = abjad.Staff(chords)
        staff2 = abjad.Staff(roots)
        group = abjad.StaffGroup([staff1, staff2])
        score = abjad.Score([group])
        abjad.show(score)

def voicelead(chart, time, seed = None, choruses = 1, slop = 1):
    last = None
    voicings = []
    roots = []
    midi = []
    indices = []
    for _ in range(choruses):
        for i, (root, chord) in enumerate(chart):
            structure = (chord + root)[1:].raw() # root is a Hierarchy
            options = [(Hierarchy(a).arrange() + n * j).flatten()
                       for a in reshapements(structure) for j in [0,1]] # allow potential registral changes
            
            if last is None:
                seed = int(random() * len(options)) if seed is None else seed
                best = (options[seed % len(options)], seed % len(options))
            else:
                distances = [distance(last, x) for x in options]
                radius = min(distances)
                bests = [(a, i) for i, a in enumerate(options) if distances[i] <= radius * slop]
                best = bests[int(random() * len(bests))]

            last = best[0]
            spelling = spell((root,) + best[0])
            
            upper = '<' + ' '.join(spelling[1:]) + f'>{time[i]}'
            voicings.append((spelling[0] + f' {time[i]}', upper))
            roots.append(root)
            midi.append(best[0])
            indices.append(best[1])

    return voicings, roots, midi, time, indices

import mido
from time import sleep

white = '\x1b[37m'
bold = '\x1b[1m'
reset = '\x1b[0m'

def background(red, green, blue):
    color = 16 + (red * 36) + (green * 6) + blue
    return f'\x1b[48;5;{color}m'

pastel = False

# chromatic
chromatic = [(background(0,1,4), background(1,1,4)),
             (background(0,2,3), background(1,2,3)),
             (background(0,3,2), background(1,3,2)),
             (background(0,4,2), background(1,4,2)), # coloring exception made here
             (background(2,4,0), background(2,4,1)), # for perceptual reasons
             (background(2,3,0), background(2,3,1)),
             (background(3,2,0), background(3,2,1)),
             (background(4,1,0), background(4,1,1)),
             (background(4,0,1), background(4,1,1)),
             (background(3,0,2), background(3,1,2)),
             (background(2,0,3), background(2,1,3)),
             (background(1,0,4), background(1,1,4))]

# circle of fifths
fifths = [chromatic[(i * 5) % n] for i in range(n)]
colors = chromatic
colors = [c[pastel] for c in colors]

colorcube = False
if colorcube:
    for red in range(6):
        for blue in range(6):
            for green in range(6):
                print(background(red, green, blue) + '  ', end = '')
            print(reset + '  ', end = '')
        
        print(reset + '  ')

# port = 'Scarlett 18i8 USB'
port = 'IAC Driver Bus 1'

tune = 'cyclic episode'
charted = True
rootless = False
if charted:
    tempo = 2.5
    driven = False
    retrig = True
    rhythmic = True
    perpetual = True
    transposition = 48
    
    X, roots, Y, rhythm, indices = voicelead(*realbook(tune), None, 3, 2)
    if not rootless: Y = [(r - n,) + y for r,y in zip(roots, Y)]
    if scoring: score(X)
else:
    tempo = 0.1
    driven = False
    retrig = True
    rhythmic = False
    perpetual = False
    transposition = 42
    
    scores = [[(0,(2,7,11,16)),
               (0,(3,8,10,19)),
               (1,(3,8,12,19)),
               (1,(2,(9,16,(18,20)))),
               ((-1,6), (1,8), (3,10)),
               ((3,10), (5,12), (7,14)),
               (3,(10,(5,12,(6,13)))),],
              [(0,(2,7,11,16)),
               (0,(2,(7,11,(16,21)))),
               ((0,2), (7,11), (16,21)),],
              reshapements((0,2,4,7,9)),
              reshapements((0,((2,3),5,7,10))),
              sum(zip(reshapements((0,(2,((4,9),11,19)))),reshapements((0,(10,((2,3),5,7))))), ()),
              sum(zip(reshapements((0,((2,3,5),7,10))), reshapements((0,(2,4,(6,7),11)))), ()),
              reshapements((0,(7,2,(9,4)))),
              reshapements((0,(2,(4,9),7))),
              [(major[:]).raw(), (major[:] + 7).raw(), (major[:]).raw(), (minor[:] + 4).raw(),
               (minor[:] + 8).raw(), (minor[:] + 9).raw(), (minor[:] + 15).raw()],
              ((0,2,4,7,9),(1,3,5,6,8,10,11))
             ]
    
    score = scores[3]
    Y = sum(([(Hierarchy(a)).arrange().flatten() for a in reshapements(b)] for b in score), start = [])
    
    print(f'{round(len(Y) * tempo / 60, 2)} minutes...')
    
total = set.union(*[set(y) for y in Y])
span = max(total) - min(total) + 1
start = min(total)

short = True
split = False

def render(Y):
    display = []
    old = ()
    for i, y in enumerate(Y):
        notenames = spell([(note + transposition) % n for note in y])
        notenames = [name[:2] if len(name) > 1 else name[0] + ' ' for name in notenames]
        new = [(note, name) for note, name in zip(y, notenames) if not note in old]
        line = ['  ' for i in range(span)]

        for note, name in zip(y, notenames):
            line[note - start] = bold + white + colors[(note + transposition) % n] + (name if (note, name) in new else '  ') + (reset if short else '')
            
        display.append(''.join(line) + reset + ('' if not charted else f' ({indices[i]})'))
        old = y
    
    return display

def play(Y, port, display):
    with mido.open_output(port) as output:
        try:
            runs = 0
            while perpetual or runs < 1:
                old = ()
                for i, (y, line) in enumerate(zip(Y, display)):
                    new = [note for note in y if not note in old]
                    print(line, end = '' if driven else '\n')
                    for note in new:
                        output.send(mido.Message('note_on', note = note + transposition, velocity = 32, channel = (note + transposition) % n if split else 0))

                    if driven: input()
                    elif rhythmic: sleep(tempo / rhythm[i % len(rhythm)])
                    else: sleep(tempo)
                    
                    for note in y:
                        output.send(mido.Message('note_off', note = note + transposition, channel = (note + transposition) % n if split else 0))

                    if retrig: old = ()
                    else: old = y
                runs += 1

        except KeyboardInterrupt:
            for note in total:
                output.send(mido.Message('note_off', note = note + transposition, channel = (note + transposition) % n if split else 0))
                sleep(0.001)
                
display = render(Y)

f = open('score.txt', 'w')
f.write('\n'.join(display))
f.close()

play(Y, port, display)

