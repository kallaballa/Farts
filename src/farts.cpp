#include "Tonic.h"
#include "kaguya/kaguya.hpp"
#include <iostream>
#include <unistd.h>
#include "cxxopts.hpp"
#include "RtAudio.h"

#include "tonic_lua.hpp"
#include "RtAudio.h"
#include "RtMidi.h"
#include "RtError.h"
#include "PolySynth.hpp"
#include "LCD.hpp"

using namespace Tonic;

const unsigned int nChannels = 2;

// Static smart pointer for our Synth
static Synth synth;
static PolySynth poly;

uint8_t current_program = 127;
size_t controlNumberOffset = 0;
LCD* lcd = nullptr;

int renderCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
		RtAudioStreamStatus status, void *userData) {
	synth.fillBufferOfFloats((float*) outputBuffer, nBufferFrames, nChannels);
	return 0;
}

void midiCallback(double deltatime, vector<unsigned char>* msg, void* userData) {
	int chan = (*msg)[0] & 0xf;
	int msgtype = (*msg)[0] & 0xf0;
	int b1 = (*msg)[1];
	int b2 = 0;
	if(msg->size() >= 2)
		b2 = (*msg)[2];

	if (msgtype == 0x80 || (msgtype == 0x90 && b2 == 0)) {
		std::cout << "MIDI Note OFF  C: " << chan << " N: " << b1 << std::endl;
		poly.noteOff(b1);
	} else if (msgtype == 0x90) {
		std::cout << "MIDI Note ON   C: " << chan << " N: " << b1 << " V: " << b2 << std::endl;
		poly.noteOn(b1, b2);
	} 	else if (msgtype == 0xB0) {
		std::cout << "MIDI CC ON     C: " << chan << " N: " << b1 << " V: " << b2 << std::endl;
		std::vector<string> commonParams;

		//try to set a common parameter for all synths. NOTE: only works if all synthesizers have the same public parameters
		if(current_program == 127 && !poly.getVoices().empty()) {
			Synth s;
			std::vector<string> currentParams;
			bool first = true;

			//check if all parameters match between all synths
			for (auto& pv : poly.getVoices()) {
				currentParams.clear();
				s = pv.synth;
				for (auto& cp : s.getParameters()) {
					const string name = cp.getDisplayName();
					//Parameters with a name starting with '_' are private
					if (!name.empty() && name.at(0) != '_') {
						currentParams.push_back(name);
					}
				}
				if(first) {
					commonParams = currentParams;
					first = false;
				} else {
					if(currentParams != commonParams) {
						std::cerr << "Synth parameters differ, can't set parameters globally" << std::endl;
						return;
					}
				}
			}

			std::vector<string> publicParameters = currentParams;

			for (auto& pv : poly.getVoices()) {
				s = pv.synth;

				if ((b1 - 52) < publicParameters.size()) {
					const string& name = publicParameters[b1 - 52];
					auto delim = name.find(".");
					string parent;
					string child;
					if(delim != string::npos) {
						parent = name.substr(0, delim);
						child = name.substr(delim+1);
					} else {
						parent = "Global";
						child = name;
					}
					if(lcd)
						lcd->clear()
							.print(0,0, parent)
							.print(1,0,child + ": " + std::to_string(b2 / 127.0f));
					s.setParameter(name, (float) b2 / 127.0);
				}
			}
		} else if(current_program < poly.getVoices().size()) {
			Synth s = poly.getVoices()[current_program].synth;
			std::vector<string> publicParameters;
			for(auto& cp : s.getParameters()) {
				const string name = cp.getDisplayName();
				if(!name.empty() && name.at(0) != '_') {
					publicParameters.push_back(name);
				}
			}
			if((b1 - 52) < publicParameters.size()) {
				const string& name = publicParameters[b1 - 52];
				std::cout << name << ": " << (float)b2/127.0 << std::endl;
				s.setParameter(name, (float)b2/127.0);
			}
		}
	} else if (msgtype == 0xC0) {
		std::cout << "MIDI Program change  C: " << chan << " P: " << b1 << std::endl;
		current_program = b1;
	}

}


int main(int argc, char ** argv) {
	std::string appName = argv[0];
	int midiIndex = 0;
	int audioIndex = 0;
  unsigned int sampleRate = 48000;
  unsigned int bufferFrames = 512;
	std::vector<string> patchFiles;
	string ttyLCD;
	cxxopts::Options options(appName, "A scriptable, modular and real-time MIDI synthesizer");
	options.add_options()
					("h,help", "Print help messages")
				  ("m,midi", "The index of the midi input device to use.", cxxopts::value<int>(midiIndex)->default_value("0"))
				  ("a,audio", "The index of the audio output device to use.", cxxopts::value<int>(audioIndex)->default_value("0"))
				  ("r,rate", "The audio output sample rate.", cxxopts::value<unsigned int>(sampleRate)->default_value("44100"))
				  ("b,buffer", "Number of frames per buffer.", cxxopts::value<unsigned int>(bufferFrames)->default_value("32"))
				  ("l,lcd", "The tty file for the LCD display.", cxxopts::value<string>(ttyLCD))
				  ("o,offset", "The control number offset for parameter mapping", cxxopts::value<size_t>(controlNumberOffset)->default_value("52"))
				  ("p,patchFiles", "The list of lua patchFiles to use as voices", cxxopts::value<std::vector<string>>(patchFiles))

					;

	auto result = options.parse(argc, argv);

	if (result["help"].count()) {
		std::cerr << "Usage: farts [options] <patch file>..."
				<< std::endl;
		std::cerr << options.help() << std::endl;
		exit(1);
	}

	if(!ttyLCD.empty()) {
		std::cerr << "Enabling LCD on " + ttyLCD << std::endl;
		lcd = new LCD(ttyLCD.c_str());
	}

	if(lcd)
		lcd->clear().print(0,0, "Starting...");
	kaguya::State state;
	bindings0(state);
	bindings1(state);
	bindings2(state);

  RtAudio dac;
   RtAudio::StreamParameters rtParams;
   rtParams.deviceId = audioIndex;
   rtParams.nChannels = nChannels;

	RtMidiIn *midiIn = new RtMidiIn();

	// You don't necessarily have to do this - it will default to 44100 if not set.
	Tonic::setSampleRate(sampleRate);
	std::vector<Synth> s(patchFiles.size());

	for (size_t i = 0; i < patchFiles.size(); ++i) {
		if(lcd)
			lcd->clear().print(0,0, "Loading: " + patchFiles[i]);
		state["synth"] = &s[i];
		state.dofile(patchFiles[i]);
		poly.addVoice(s[i]);
	}

	//add a slight ADSR to prevent clicking
	synth.setOutputGen(poly);

	// open rtaudio stream and rtmidi port
	try {
		if (midiIn->getPortCount() == 0) {
			std::cerr << "No MIDI ports available!\n";
			cin.get();
			exit(0);
		}
		std::cerr << "Opening MIDI port: " + midiIndex << std::endl;
		midiIn->openPort(midiIndex);
		midiIn->setCallback(&midiCallback);

		std::cerr << "Opening audio port: " << rtParams.deviceId <<
				" channels: "	<< rtParams.nChannels <<
				" rate: " << sampleRate <<
				" frames: "	<< bufferFrames << std::endl;

    dac.openStream( &rtParams, NULL, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &renderCallback, NULL, NULL );
    dac.startStream();
		if(lcd)
			lcd->clear().print(0,0, "Running");

	while(true) { sleep(10); };		
dac.stopStream();
	} catch (RtError& e) {
		std::cout << '\n' << e.getMessage() << '\n' << std::endl;
		cin.get();
		exit(0);
	}
	return 0;
}

