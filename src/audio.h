// audio.h
#include "includes.h"

#ifndef AUDIO

#ifdef PORTAUDIO
namespace soundmath
{
	const int def_bsize = 16;
	inline int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

	class Audio
	{
	public:
		int bsize;
		PaStream* stream;
		PaStreamParameters inParams, outParams;
		int (*process)(const float*, float*);

		Audio(int (*processor)(const float* in, float* out), int bsize = def_bsize) : bsize(bsize)
		{
			process = processor;
		}

		static PaError clean(PaError err)
		{
			if (err != paNoError)
			{
				Pa_Terminate();
				std::cout << "PortAudio error: " << Pa_GetErrorText(err);
			}
			return err;
		}

		void startup(int in = 1, int out = 2, bool report = true)
		{
			clean(Pa_Initialize());

			const PaDeviceInfo* deviceInfo;

			inParams.device = Pa_GetDefaultInputDevice();
			if (inParams.device == paNoDevice)
			{
				std::cout << "Error: No default input device.\n";
				shutdown();
			}
			deviceInfo = Pa_GetDeviceInfo(inParams.device);
			inParams.channelCount = in;
			inParams.sampleFormat = paFloat32;
			inParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
			inParams.hostApiSpecificStreamInfo = NULL;

			if (report)
			{
				std::cout << "Input from " << deviceInfo->name << "\n";
				std::cout << "  " << deviceInfo->maxInputChannels << " channels available.\n";
			}

			outParams.device = Pa_GetDefaultOutputDevice();
			if (outParams.device == paNoDevice)
			{
				std::cout << "Error: No default output device.\n";
				shutdown();
			}
			deviceInfo = Pa_GetDeviceInfo(outParams.device);
			outParams.channelCount = out;
			outParams.sampleFormat = paFloat32;
			outParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
			outParams.hostApiSpecificStreamInfo = NULL;

			if (report)
			{
				std::cout << "Output to " << deviceInfo->name << "\n";
				std::cout << "  " << deviceInfo->maxOutputChannels << " channels available.\n";
			}

			clean(Pa_OpenStream(&stream, &inParams, &outParams, SR, bsize, 0, callback, this));
			clean(Pa_StartStream(stream));
		}

		void shutdown()
		{
			clean(Pa_StopStream(stream));
			clean(Pa_CloseStream(stream));
			clean(Pa_Terminate());
		}
	};

	int callback(const void* inputBuffer, void* outputBuffer, 
				  unsigned long framesPerBuffer, 
				  const PaStreamCallbackTimeInfo* timeInfo, 
				  PaStreamCallbackFlags statusFlags,
				  void* userData)
	{
		Audio* A = (Audio*)userData;
		A->process((const float*) inputBuffer, (float*) outputBuffer);
		return 0;
	}
}
#else
namespace soundmath
{
	const int def_bsize = 16;
	inline int callback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

	class Audio
	{
	public:
		int bsize;
		PaStream* stream;
		PaStreamParameters inParams, outParams;
		int (*process)(const float*, float*);

		Audio(int (*processor)(const float* in, float* out), int bsize = def_bsize) : bsize(bsize)
		{
			process = processor;
		}

		static PaError clean(PaError err)
		{
			if (err != paNoError)
			{
				Pa_Terminate();
				std::cout << "PortAudio error: " << Pa_GetErrorText(err);
			}
			return err;
		}

		void startup(int in = 1, int out = 2, bool report = true)
		{
			clean(Pa_Initialize());

			const PaDeviceInfo* deviceInfo;

			inParams.device = Pa_GetDefaultInputDevice();
			if (inParams.device == paNoDevice)
			{
				std::cout << "Error: No default input device.\n";
				shutdown();
			}
			deviceInfo = Pa_GetDeviceInfo(inParams.device);
			inParams.channelCount = in;
			inParams.sampleFormat = paFloat32;
			inParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
			inParams.hostApiSpecificStreamInfo = NULL;

			if (report)
			{
				std::cout << "Input from " << deviceInfo->name << "\n";
				std::cout << "  " << deviceInfo->maxInputChannels << " channels available.\n";
			}

			outParams.device = Pa_GetDefaultOutputDevice();
			if (outParams.device == paNoDevice)
			{
				std::cout << "Error: No default output device.\n";
				shutdown();
			}
			deviceInfo = Pa_GetDeviceInfo(outParams.device);
			outParams.channelCount = out;
			outParams.sampleFormat = paFloat32;
			outParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
			outParams.hostApiSpecificStreamInfo = NULL;

			if (report)
			{
				std::cout << "Output to " << deviceInfo->name << "\n";
				std::cout << "  " << deviceInfo->maxOutputChannels << " channels available.\n";
			}

			clean(Pa_OpenStream(&stream, &inParams, &outParams, SR, bsize, 0, callback, this));
			clean(Pa_StartStream(stream));
		}

		void shutdown()
		{
			clean(Pa_StopStream(stream));
			clean(Pa_CloseStream(stream));
			clean(Pa_Terminate());
		}
	};

	int callback(const void* inputBuffer, void* outputBuffer, 
				  unsigned long framesPerBuffer, 
				  const PaStreamCallbackTimeInfo* timeInfo, 
				  PaStreamCallbackFlags statusFlags,
				  void* userData)
	{
		Audio* A = (Audio*)userData;
		A->process((const float*) inputBuffer, (float*) outputBuffer);
		return 0;
	}
}
#endif

#define AUDIO
#endif