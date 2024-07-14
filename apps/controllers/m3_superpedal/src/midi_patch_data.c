/*
 * MIDI Patch data
 * Copyright Scott Rush 2024
 */
 /////////////////////////////////////////////////////////////////////////////
 // Include files
 /////////////////////////////////////////////////////////////////////////////

#include "midi_patch_data.h"

//-------------------------------------------------------------------------------
// General MIDI bank and Voice Names
const char* defaultGenMIDIBankNames[NUM_GEN_MIDI_BANK_NAMES] = { "Strings","Bass","Keys","Wind/Pads" };

const char* genMIDIVoiceNames[NUM_GEN_MIDI_VOICE_NAMES] = { 
"Acoustic Grand Piano","Bright Acoustic Piano","Electric Grand Piano","Honky-tonk Piano","Electric Piano 1",
"Electric Piano 2","Harpsichord","Clavi","Celesta","Glockenspiel",
"Music Box","Vibraphone","Marimba","Xylophone","Tubular Bells",
"Dulcimer","Drawbar Organ","Percussive Organ","Rock Organ","Church Organ",
"Reed Organ","Accordion","Harmonica","Tango Accordion","Nylon Acoustic Guitar",
"Acoustic Guitar (steel)","Electric Guitar (jazz)","Clean Electric Guitar","Muted Electric Guitar","Overdriven Guitar",
"Distortion Guitar","Guitar harmonics","Acoustic Bass","Finger Electric Bass","Picked Electric Bass",
"Fretless Bass","Slap Bass 1","Slap Bass 2","Synth Bass 1","Synth Bass 2",
"Violin","Viola","Cello","Contrabass","Tremolo Strings",
"Pizzicato Strings","Orchestral Harp","Timpani","String Ensemble 1","String Ensemble 2",
"SynthStrings 1","SynthStrings 2","Choir Aahs","Voice Oohs","Synth Voice",
"Orchestra Hit","Trumpet","Trombone","Tuba","Muted Trumpet",
"French Horn","Brass Section","SynthBrass 1","SynthBrass 2",
"Soprano Sax","Alto Sax","Tenor Sax","Baritone Sax","Oboe","English Horn",
"Bassoon","Clarinet","Piccolo","Flute","Recorder",
"Pan Flute","Blown Bottle","Shakuhachi","Whistle","Ocarina",
"Lead 1 (square)","Lead 2 (sawtooth)","Lead 3 (calliope)","Lead 4 (chiff)","Lead 5 (charang)",
"Lead 6 (voice)","Lead 7 (fifths)","Lead 8 (bass + lead)","Pad 1 (new age)","Pad 2 (warm)",
"Pad 3 (polysynth)","Pad 4 (choir)","Pad 5 (bowed)","Pad 6 (metallic)","Pad 7 (halo)",
"Pad 8 (sweep)","FX 1 (rain)","FX 2 (soundtrack)","FX 3 (crystal)","FX 4 (atmosphere)",
"FX 5 (brightness)","FX 6 (goblins)","FX 7 (echoes)","FX 8 (sci-fi)","Sitar",
"Banjo","Shamisen","Koto","Kalimba","Bag pipe",
"Fiddle","Shanai","Tinkle Bell","Agogo","Steel Drums",
"Woodblock","Taiko Drum","Melodic Tom","Synth Drum","Reverse Cymbal",
"Guitar Fret Noise","Breath Noise","Seashore","Bird Tweet","Telephone Ring",
"Helicopter","Applause","Gunshot" };
//------------ End of General MIDI

//-------------------------------------------------------------------------------
// Disklavier XG General MIDI with bank numbers
//-------------------------------------------------------------------------------

const disklavier_midi_patch_t disklavierPatches[NUM_DISKLAVIER_PATCHES];


//------------- End of Disklavier MIDI patch

//-------------------------------------------------------------------------------

const char* jv880_ABPresets[] = { "Acoustic Piano 1","Acoustic Piano 2","Mellow Piano","Pop Piano 1","Pop Piano 2",
"Pop Piano 3","MIDIed Grand","Country Bar","Glist EPiano","MIDI EPiano",
"SA Rhodes","Dig Rhodes 1","Dig Rhodes 2","Stiky Rhodes","Guitr Rhodes",
"Nylon Rhodes","Clav 1","Clav 2","Marimba","Marimba SW",
"Warm Vibe","Vibe","Wave Bells","Vibrobell","Pipe Organ 1",
"Pipe Organ 2","Pipe Organ 3","Electric Organ 1","Electric Organ 2","Jazz Organ 1",
"Jazz Organ 2","Metal Organ","Nylon Gtr 1","Flanged Nyln","Steel Guitar",
"PickedGuitar","12 strings","Velo Harmnix","Nylon+Steel","SwitchOnMute",
"JC Strat","Stratus","Syn Strat","Pop Strat","Clean Strat",
"Funk Gtr","Syn Guitar","Overdrive","Fretless","St Fretless",
"Woody Bass 1","Woody Bass 2","Analog Bs 1","House Bass","Hip Bass",
"RockOut Bass","Slap Bass","Thumpin Bass","Pick Bass","Wonder Bass",
"Yowza Bass","Rubber Bass 1","Rubber Bass 2","Stereoww Bs","Pizzicato",
"Real Pizz","Harp","SoarinString","Warm Strings","Marcato",
"St Strings","Orch Strings","Slow Strings","Velo Strings","BrightStrngs",
"TremoloStrng","Orch Stab 1","Brite Stab","JP-8 Strings","String Synth",
"Wire Strings","New Age Vox","Arasian Morn","Beauty Vox","Vento Voxx",
"Pvox Oooze","GlassVoices","Space Ahh","Trumpet","Trombone",
"Harmon Mute1","Harmon Mute2","TeaJay Brass","Brass Sect 1","Brass Sect 2",
"Brass Swell","Brass Combo","Stab Brass","Soft Brass","Horn Brass",
"French Horn","AltoLead Sax","Alto Sax","Tenor Sax 1","Tenor Sax 2",
"Sax Section","Sax Tp Tb","FlutePiccolo","Flute mod","Ocarina",
"OverblownPan","Air Lead","Steel Drum","Log Drum","Box Lead",
"Soft Lead","Whistle","Square Lead","Touch Lead","NightShade",
"Pizza Hutt","EP+Exp Pad","JP-8 Pad","Puff","SpaciosSweep",
"Big n Beefy","RevCymBend","Analog Seq" };

const char* jv880_InternalPresets[] = { "770 Grand 1","MIDI 3 Grand","Electric Grand 1","60s Electric Piano","Dyna Rhodes",
"Pop Rhodes","Beauty Rhodes","Airies Piano","Clav 1 x4","Wah Clav",
"Housey Clavy","Ballad Org.1","Even Bars 1","Stereo Organ","Jazz Organ 3",
"8ft. Stop","Brite Organ 1","Soft Organ","60s Organ x4","Pipe Organ 4",
"Church","Celeste","Toy Piano","Snow Bells","Acoustic Bass 1",
"Acoustic Bass 2","Acoustic Fretless","Fretless 2","Weather Bass","Jazz Bass",
"Power Bass","Power Funk V-Sw","Stick","Delicate Stik","Stick V-Sw",
"Bassic house","Noo Spitbass","Metal Bass","Sync Bass","Bs Slide",
"Bs Harmonix","Super Nylon","Jazz Guitar","Jazz Cascade","Jazzy Scat",
"Pedal Steel","Banjo 1","Electric Sitar","Funk Guitar","Heavy Duty",
"Lead Guitar 1","Lead Guitar 2","Lead Guitar 3","Pocket Rocket","Power Flange",
"Bowed Guitar","Shakupeace","Cimbalon 1","Sanza 1","Shamisentur",
"Praying Monk","Electric Koto","Ravi Sitar","Mystic Mount" };

