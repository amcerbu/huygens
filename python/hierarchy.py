import math
n = 12

# build cache dictionary while recursing?
cacheall = True

# voice-leading distance. nb: needs p and q sorted!
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
                # weight = (sum(x) * sum(y) / (len(x) * len(y))) # over-penalize large intervals in high register
                weight = 1
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
    
    def reduce(self, octave = n):
        if self.atom: return {self.data % octave}
        return set.union(*(a.reduce() for a in self))
    
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