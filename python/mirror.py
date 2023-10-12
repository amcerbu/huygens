import sys
import mido

ports = mido.get_input_names()

print('ports: ' + str(ports))
print('')

surface = None
synth = None

if len(sys.argv) == 2:
	surface = sys.argv[1]
elif len(sys.argv) > 2:
	surface = sys.argv[1]
	synth = sys.argv[2]

print('in: ' + str(surface) + ', out: ' + str(synth))

allowed = ['note_off', 'note_on']
print(f'Allowed messages: {allowed}')

with mido.open_input(surface) as ioport, \
	 mido.open_output(synth) as output:
	try:
		for message in ioport:
			if message.type in allowed:
				print(message)
				output.send(message)

	except KeyboardInterrupt:
		pass
	

'''
 - draw your own velocity curve
 - chord / voicing mode -- store pitch-class sets in banks (maybe also provide substitutions)
 	- rootless mode (roots do not sound)
 	- clockface mode (draw clock logo for chord storage)
 	- stored chords may be endowed list of permissible roots
 - 
'''
