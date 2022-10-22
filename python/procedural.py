import numpy as np
from random import random
from math import sqrt
import math
from hierarchy import Hierarchy, distance, permutations, reshapements
from scoring import *
import mido
from time import sleep
from time import time as microseconds

n = 12

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


def realbook(tune):
    if tune == 'giant steps':
        chart = [(B, major), (D, dominant), (G, major), (Bf, dominant), (Ef, major), (A, minor), (D, dominant),
                 (G, major), (Bf, dominant), (Ef, major), (Fs, dominant), (B, major), (F, minor), (Bf, dominant),
                 (Ef, major), (A, minor), (D, dominant), (G, major), (Cs, minor), (Fs, dominant), (B, major),
                 (F, minor), (Bf, dominant), (Ef, major), (Df, minor), (Gf, dominant)]

        time = [4, 4, 4, 4, 2, 4, 4,
                4, 4, 4, 4, 2, 4, 4,
                2, 4, 4, 2, 4, 4, 2, 4, 4, 2, 4, 4]

    elif tune == 'cyclic episode':
        chart = [(Bf, minor), (Df, minor), (E, minor), (G, minor),
                 (C, minor), (D, alts9), (G, minor), (A, alts9),
                 (D, minor), (B, minor), (Af, minor), (F, minor),
                 (C, major), (C, minor), (Ef, minor), (Gf, minor)]

        time = [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]

    elif tune == 'there will never be another you':
        chart = [(Ef, major), (D, halfdim), (G, domf9),
                 (C, minor), (Bf, minor), (Ef, domf9),
                 (Af, major), (Df, dominant), (Ef, major), (C, minor),
                 (F, dominant), (F, minor), (Bf, domf9),
                 (Ef, major), (D, halfdim), (G, domf9),
                 (C, minor), (Bf, minor), (Ef, dominant),
                 (Af, major), (Df, dominant), (Ef, major), (A, minor), (D, dominant),
                 (Ef, major), (D, domf9), (G, minor), (C, dominant), (F, minor), (Bf, dominant), (Ef, major)]

        time = [1, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 2]

    elif tune == 'moment\'s notice':
        chart = [(E, minor), (A, dominant), (F, minor), (Bf, dominant), (Ef, major), (Af, minor), (Df, dominant),
                 (D, minor), (G, dominant), (Ef, minor), (Af, dominant), (Df, major), (D, minor), (G, dominant),
                 (C, minor), (Bf, minor), (Ef, dominant), (Af, major), (Df, dominant),
                 (G, minor), (C, minor), (Af, minor), (Df, dominant), (Gf, major), (F, minor), (Bf, dominant),
                 (E, minor), (A, dominant), (F, minor), (Bf, dominant), (Ef, major), (Af, minor), (Df, dominant),
                 (D, minor), (G, dominant), (Ef, minor), (Af, dominant), (Df, major), (D, minor), (G, dominant),
                 (C, minor), (Bf, minor), (Ef, dominant), (Af, major), (Df, dominant),
                 (G, minor), (C, minor), (F, minor), (Bf, dominant), (Ef, major), (F, minor),
                 (G, minor), (F, minor), (Ef, major), (F, minor), (G, minor), (F, minor), (Ef, major),(Ef, major)]

        time = [2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 1, 2, 2, 
                2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1]

    elif tune == 'it could happen to you':
        chart = [(Ef, major), (E, diminished), (F, minor), (Fs, diminished), 
                 (Ef, major), (Af, major), (G, halfdim), (C, domf9),
                 (F, minor), (Df, dominant), (Ef, major), (D, halfdim), (G, domf9),
                 (C, minor), (F, dominant), (F, minor), (Bf, dominant),
                 (Ef, major), (E, diminished), (F, minor), (Fs, diminished), 
                 (Ef, major), (Af, major), (G, halfdim), (C, domf9),
                 (F, minor), (Df, dominant), (Ef, major), (Af, domf9), (G, halfdim), (C, dominant),
                 (F, minor), (Bf, domf9), (Ef, major), (C, minor), (F, minor), (Bf, dominant)]

        time = [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 2, 2, 2, 2, 
                2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 2, 2, 4, 4, 4, 4] 

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

white = '\x1b[37m'
bold = '\x1b[1m'
reset = '\x1b[0m'

def background(red, green, blue):
    color = 16 + (red * 36) + (green * 6) + blue
    return f'\x1b[48;5;{color}m'

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

colors = chromatic
pastel = False
colors = [c[pastel] for c in colors]

def colorcube():
    for red in range(6):
        for blue in range(6):
            for green in range(6):
                print(background(red, green, blue) + '  ', end = '')
            print(reset + '  ', end = '')
        
        print(reset + '  ')

def render(Y, transposition, charted, indices = None, short = True):
    total = set.union(*[set(y) for y in Y])
    span = max(total) - min(total) + 1
    start = min(total)

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

def play(Y, transposition, tempo, port, display, perpetual, driven, rhythmic, rhythm = None, split = False, retrig = True):
    total = set.union(*[set(y) for y in Y])
    
    with mido.open_output(port) as output:
        try:
            runs = 0
            last = microseconds()
            sleep(tempo)
            while perpetual or runs < 1:
                old = ()
                for i, (y, line) in enumerate(zip(Y, display)):
                    now = microseconds()
                    
                    new = [note for note in y if not note in old]
                    print(line, end = '' if driven else '\n')
                    for note in new:
                        output.send(mido.Message('note_on', note = note + transposition, velocity = 32, channel = (note + transposition) % n if split else 0))
                        # sleep(tempo)

                    if driven: input()
                    elif rhythmic:
                        delay = tempo / rhythm[i % len(rhythm)]
                    else:
                        delay = tempo

                    elapsed = now - last
                    last = now
                    # sleep(delay - (elapsed - delay))
                    sleep(delay)
                    
                    for note in y:
                        output.send(mido.Message('note_off', note = note + transposition, channel = (note + transposition) % n if split else 0))

                    if retrig: old = ()
                    else: old = y
                runs += 1

        except KeyboardInterrupt:
            for note in total:
                output.send(mido.Message('note_off', note = note + transposition, channel = (note + transposition) % n if split else 0))
                sleep(0.001)


if __name__ == "__main__":
    # port = 'Scarlett 18i8 USB'
    port = 'IAC Driver Bus 1'
    tune = 'cyclic episode'
    charted = False
    rootless = False
    if charted:
        tempo = 2.5
        driven = False
        rhythmic = True
        perpetual = True
        transposition = 36
        
        seed = 6 # None
        X, roots, Y, rhythm, indices = voicelead(*realbook(tune), seed, 3, 2)
        if not rootless: Y = [(r - n,) + y for r,y in zip(roots, Y)]

        display = render(Y, transposition, charted, indices)

    else:
        # tempo = 0.1
        tempo = 0.1
        driven = False
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
                  sum(zip(reshapements((0,((2,(3,(0, 5))),7,10))), reshapements((0,(2,4,(6,7),11)))), ()),
                  reshapements((0,(7,2,(9,4)))),
                  reshapements((0,(2,(4,9),7))),
                  [(major[:]).raw(), (major[:] + 7).raw(), (major[:]).raw(), (minor[:] + 4).raw(),
                   (minor[:] + 8).raw(), (minor[:] + 9).raw(), (minor[:] + 15).raw()],
                  ((0,2,4,7,9),(1,3,5,6,8,10,11))
                 ]
        
        score = scores[5]
        Y = sum(([(Hierarchy(a)).arrange().flatten() for a in reshapements(b)] for b in score), start = [])

        display = render(Y, transposition, charted)
        rhythm = None
        
        print(f'{round(len(Y) * tempo / 60, 2)} minutes...')
        
    play(Y, transposition, tempo, port, display, perpetual, driven, rhythmic, rhythm, True)