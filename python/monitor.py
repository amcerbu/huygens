import sys
import mido

ports = mido.get_input_names()

print('ports: ' + str(ports))
print('')

surface = None

if len(sys.argv) > 1:
	surface = sys.argv[1]

print('in: ' + str(surface))

with mido.open_input(surface) as ioport, \
	 mido.open_output(synth) as output:
	try:
		for message in ioport:
			print(message)

	except KeyboardInterrupt:
		pass
