n = 12

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