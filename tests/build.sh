#!/usr/bin/env zsh
# usage: ./build name.cpp to build executable Name, or ./build to build all *.cpp in directory

base=${1%.cpp}

build () {
	# g++ -Wall -O3 -std=c++17 $1 -o $outname -I /usr/local/include/eigen3 -I /usr/local/include/rtmidi -l m -l fftw3 -l portaudio -l ncurses -l rtmidi -include ../src/tinyosc.cpp # -l soundio
	g++ -Wall -O3 -std=c++17 $1 -o $outname -I /opt/homebrew/include -I /opt/homebrew/include/eigen3 -I /opt/homebrew/include/rtmidi -L /opt/homebrew/lib -l m -l fftw3 -l portaudio -l ncurses -l rtmidi -include ../src/tinyosc.cpp # -l soundio

}

if [ -z "$base" ]
then
	for i in *.cpp; do
		[ -f "$i" ] || break
		base=${i%.cpp}
		echo -n "building $i ... "
		outname=${(C)base}
		build $i
		echo "wrote ${outname}"
	done
else
	echo -n "building $1 ... "
	outname=${(C)base}
	build $1
	echo "wrote ${outname}"
fi