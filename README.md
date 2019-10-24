Farts is a scriptable modular synthesizer. The scripting is done in [lua](https://www.lua.org) and it basically exposes [Tonic](https://github.com/TonicAudio/Tonic) as literal as possible to the scripts. At the moment that is used in a simple midi synthesizer implementation, but with a little effort it can be used in a lot of scenarios (like a standalone syntesizer based on pathes or in combination with a sequencer).

#Usage

    src/farts [midi port index] [control number offset] [syntesizer patch]...


for example to create a midi synthesizer with 3 voices connecting to the second midi port (port index starts at 0) which automatically maps patch parameters to control numbers starting at the control number offset in ascending order:

    src/farts 1 52 src/sin.lua src/sin.lua src/sin.lua src/sin.lua

Note that you can use a different patch per voice.

#Example patch

The following code creates a MIDI patch with a square wave tone and an ADSR envelope. every parameter added with "addParameter" will be audomatically exposed to the midi interface except those starting with '_' (those are private). It very closely resembles the Tonic C++ API.

    local noteNum = synth:addParameter("_polyNote", 0.0);
    local gate = synth:addParameter("_polyGate", 0.0);
    local noteVelocity = synth:addParameter("_polyVelocity", 0.0);
    local voiceNumber = synth:addParameter("_polyVoiceNumber", 0.0);
    
    local voiceFreq = ControlMidiToFreq():input(noteNum)
    local tone = SquareWave():freq(voiceFreq)
    local fv = FixedValue(0.02)
    local env = ADSR()
        :attack(synth:addParameter("attack",0.04))
        :decay(synth:addParameter("decay", 0.1 ))
        :sustain(synth:addParameter("sustain",0.8))
        :release(synth:addParameter("release",0.6))
        :doesSustain(1)
        :trigger(gate);

    synth:setOutputGen((tone * env) * (fv + noteVelocity * 0.005));

#Build

At the moment only linux is supported but it shouldn't be hard to port it to other platforms.

Dependencies:
- libasound (ALSA)
- libpulse (PulseAudio) 
- libpulse-simple (Pulse-simple)
- libreadline


After you installed the dependencies all you have to do is type

    ./build_third.sh
    make

