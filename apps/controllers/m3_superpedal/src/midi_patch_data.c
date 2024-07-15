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


//-------------------------------------------------------------------------------
// Disklavier XG General MIDI with bank numbers
//-------------------------------------------------------------------------------


const char* disklavierGroupNames[NUM_DISKLAVIER_GROUPS] =
{ "Piano","Chromatic Percussion","Organ","Guitar","Bass","Strings","Ensemble","Brass","Reed","Pipe",
"Synth Lead","Synth Pad","Synth Effects","Ethnic","Percussive","Sound Effects", };

const disklavier_voice_midi_number_t disklavierVoiceMIDINumbers[NUM_DISKLAVIER_VOICES] = {
   //----------------------------------------------------------------
   // Start of Piano group
   // Program Number: 1 'Grand Piano'
   {1, 1,0},  //-- 'Grand Piano'
   {1, 1,1},  //-- 'Grand Piano K'
   {1, 1,18},  //-- 'Mello Grand Piano'
   {1, 1,40},  //-- 'Piano Strings'
   {1, 1,41},  //-- 'Piano Dream'
   // Program Number: 2 'Brite Piano'
   {1, 2,0},  //-- 'Brite Piano'
   {1, 2,1},  //-- 'Brite Piano K'
   // Program Number: 3 'Elec Grand Piano'
   {1, 3,0},  //-- 'Elec Grand Piano'
   {1, 3,1},  //-- 'Elec Grand Piano K'
   {1, 3,32},  //-- 'Det. CP80'
   {1, 3,40},  //-- 'Elec Grand Piano 1'
   {1, 3,41},  //-- 'Elec Grand Piano 2'
   // Program Number: 4 'Honky tonk Piano 2'
   {1, 4,0},  //-- 'Honky tonk Piano 2'
   {1, 4,1},  //-- 'Honky Tonk Piano K 2'
   // Program Number: 5 'Elec Piano 1'
   {1, 5,0},  //-- 'Elec Piano 1'
   {1, 5,1},  //-- 'Elec Piano K 1'
   {1, 5,18},  //-- 'Mello Elec Piano 1'
   {1, 5,32},  //-- 'Chorus Elec Piano 1'
   {1, 5,40},  //-- 'Hard Elec Piano 1'
   {1, 5,45},  //-- 'VX Elec Piano 1'
   {1, 5,64},  //-- '60s Elec Pianao'
   // Program Number: 6 'Elec Piano 2'
   {1, 6,0},  //-- 'Elec Piano 2'
   {1, 6,1},  //-- 'Elec Piano 2 K'
   {1, 6,32},  //-- 'Chorus Elec Piano 2'
   {1, 6,33},  //-- 'DX Hard'
   {1, 6,34},  //-- 'DX Legend'
   {1, 6,40},  //-- 'DX Phase'
   {1, 6,41},  //-- 'DX+Analog'
   {1, 6,42},  //-- 'DX Koto Elec Piano'
   {1, 6,45},  //-- 'VX Elec Piano 2'
   // Program Number: 7 'Harpsichord'
   {1, 7,0},  //-- 'Harpsichord'
   {1, 7,1},  //-- 'Harpsichord K'
   {1, 7,25},  //-- 'Harpsichord 2'
   {1, 7,35},  //-- 'Harpsichord 3'
   // Program Number: 8 'Clavinet'
   {1, 8,0},  //-- 'Clavinet'
   {1, 8,1},  //-- 'Clavinet K'
   {1, 8,27},  //-- 'Clavinet Wah'
   {1, 8,64},  //-- 'Pulse Clavinet'
   {1, 8,65},  //-- 'Pierce Clavinet'

   //--------- Chromatic Percussion group --------------------------
   // Program Number: 9 'Celesta'
   {2, 9,0},  //-- 'Celesta'
   // Program Number: 10 'Glocken'
   {2, 10,0},  //-- 'Glocken'
   // Program Number: 11 'Music Box'
   {2, 11,0},  //-- 'Music Box'
   {2, 11,64},  //-- 'Orgel'
   // Program Number: 12 'Vibes'
   {2, 12,0},  //-- 'Vibes'
   {2, 12,1},  //-- 'Vibes K'
   {2, 12,45},  //-- 'Hard Vibe'
   // Program Number: 13 'Marimba'
   {2, 13,0},  //-- 'Marimba'
   {2, 13,1},  //-- 'Marimba K'
   {2, 13,64},  //-- 'Sine Marimba'
   {2, 13,97},  //-- 'Balafone'
   {2, 13,98},  //-- 'Log Drum'
   // Program Number: 14 'Xylophone'
   {2, 14,0},  //-- 'Xylophone'
   // Program Number: 15 'Tubular Bell'
   {2, 15,0},  //-- 'Tubular Bell'
   {2, 15,96},  //-- 'Church Bell'
   {2, 15,97},  //-- 'Carillon'
   // Program Number: 16 'Dulcimer'
   {2, 16,0},  //-- 'Dulcimer'
   {2, 16,35},  //-- 'Dulcimer 2'
   {2, 16,96},  //-- 'Cimbalom'
   {2, 16,97},  //-- 'Santur'

   //--------- Organ group --------------------------
   // Program Number: 17 'Drawbar Organ'
   {3, 17,0},  //-- 'Drawbar Organ'
   {3, 17,32},  //-- 'Det Draw Organ'
   {3, 17,33},  //-- '60s Draw Organ 1'
   {3, 17,34},  //-- '60s Draw Organ 2'
   {3, 17,35},  //-- '70s Draw Organ 1'
   {3, 17,36},  //-- ' Drawbar Organ 2'
   {3, 17,37},  //-- '60s Draw Organ 3'
   {3, 17,38},  //-- 'Even Bar Organ'
   {3, 17,40},  //-- '16+2"2/3 Organ"'
   {3, 17,64},  //-- 'Organ Bar'
   {3, 17,65},  //-- '70s Draw Organ 2'
   {3, 17,66},  //-- 'Cheeze Organ'
   {3, 17,67},  //-- 'Draw Organ 3'
   // Program Number: 18 'PercussionOrgan'
   {3, 18,0},  //-- 'PercussionOrgan'
   {3, 18,24},  //-- '70s Perc Organ'
   {3, 18,32},  //-- 'Det Perc Organ'
   {3, 18,33},  //-- 'Lite Organ'
   {3, 18,37},  //-- 'Percussion Organ 2'
   // Program Number: 19 'Rock Organ'
   {3, 19,0},  //-- 'Rock Organ'
   {3, 19,64},  //-- 'Rotary Organ'
   {3, 19,65},  //-- 'Slow Rotary Organ'
   {3, 19,66},  //-- 'Fast Rotary Organ'
   // Program Number: 20 'Church Organ'
   {3, 20,0},  //-- 'Church Organ'
   {3, 20,32},  //-- 'Church Organ 3'
   {3, 20,35},  //-- 'Church Organ 2'
   {3, 20,40},  //-- 'Notre Dame Organ'
   {3, 20,64},  //-- 'Organ Flute'
   {3, 20,65},  //-- 'Trm Organ Flute'
   // Program Number: 21 'Reed Organ'
   {3, 21,0},  //-- 'Reed Organ'
   {3, 21,40},  //-- 'Puff Organ'
   // Program Number: 22 'Accordion'
   {3, 22,0},  //-- 'Accordion'
   {3, 22,32},  //-- 'Accodian Light'
   // Program Number: 23 'Harmonica'
   {3, 23,0},  //-- 'Harmonica'
   {3, 23,32},  //-- 'Harmonica 2'
   // Program Number: 24 'Tango Accordian'
   {3, 24,0},  //-- 'Tango Accordian'
   {3, 24,64},  //-- 'Tango Accordian 2'

   //--------- Guitar group --------------------------
   // Program Number: 25 'Nylon Guitar'
   {4, 25,0},  //-- 'Nylon Guitar'
   {4, 25,16},  //-- 'Nylon Guitar 2'
   {4, 25,25},  //-- 'Nylon Guitar 3'
   {4, 25,43},  //-- 'Vel Guitar Harmonic'
   {4, 25,96},  //-- 'Ukulele'
   // Program Number: 26 'Steel Guitar'
   {4, 26,0},  //-- 'Steel Guitar'
   {4, 26,16},  //-- 'Steel Guitar 2'
   {4, 26,35},  //-- '12 String Guitar'
   {4, 26,40},  //-- 'Nylon & Steel Guitar'
   {4, 26,41},  //-- 'Steel & Body Guitar'
   {4, 26,96},  //-- 'Mandolin'
   // Program Number: 27 'Jazz Guitar'
   {4, 27,0},  //-- 'Jazz Guitar'
   {4, 27,18},  //-- 'Mello Guitar'
   {4, 27,32},  //-- 'Jazz Amp Guitar'
   // Program Number: 28 'Clean Guitar'
   {4, 28,0},  //-- 'Clean Guitar'
   {4, 28,32},  //-- 'Chorus Guitar'
   // Program Number: 29 'Mute Guitar'
   {4, 29,0},  //-- 'Mute Guitar'
   {4, 29,40},  //-- 'Funk Guitar'
   {4, 29,41},  //-- 'Mute Steel Guitar'
   {4, 29,43},  //-- 'Funk Guitar 2'
   {4, 29,45},  //-- 'Jazz Man Guitar'
   // Program Number: 30 'Overdrive Guitar'
   {4, 30,0},  //-- 'Overdrive Guitar'
   {4, 30,43},  //-- 'Guitar Pinch'
   // Program Number: 31 'Distortion Guitar'
   {4, 31,0},  //-- 'Distortion Guitar'
   {4, 31,40},  //-- 'Feedback Guitar'
   {4, 31,41},  //-- 'Feedback Guitar 2'
   // Program Number: 32 'Guitar Harmonics'
   {4, 32,0},  //-- 'Guitar Harmonics'
   {4, 32,65},  //-- 'Guitar Feedback'
   {4, 32,66},  //-- 'Guitar Harmonics 2'

   //--------- Bass group --------------------------
   // Program Number: 33 'Acoustic Bass'
   {5, 33,0},  //-- 'Acoustic Bass'
   {5, 33,40},  //-- 'Jazz Rhythm Bass'
   {5, 33,45},  //-- 'VX Upright Bass'
   // Program Number: 34 'Fingered Bass'
   {5, 34,0},  //-- 'Fingered Bass'
   {5, 34,18},  //-- 'Fingered Dark Bass'
   {5, 34,27},  //-- 'Flange Bass'
   {5, 34,40},  //-- 'Bass & Dist Elec Guitar'
   {5, 34,43},  //-- 'Finger Slap Bass'
   {5, 34,45},  //-- 'Fingered Bass 2'
   {5, 34,65},  //-- 'Mod Alem Bass'
   // Program Number: 35 'Picked Bass'
   {5, 35,0},  //-- 'Picked Bass'
   {5, 35,28},  //-- 'Mute Picked Bass'
   // Program Number: 36 'Fretless Bass'
   {5, 36,0},  //-- 'Fretless Bass'
   {5, 36,32},  //-- 'Fretless Bass 2'
   {5, 36,33},  //-- 'Fretless Bass 3'
   {5, 36,34},  //-- 'Fretless Bass 4'
   {5, 36,96},  //-- 'Syn Fretfless Bass'
   {5, 36,97},  //-- 'Smooth Bass'
   // Program Number: 37 'Slap Bass'
   {5, 37,0},  //-- 'Slap Bass'
   {5, 37,27},  //-- 'Resonant Slap Bass'
   {5, 37,32},  //-- 'Punch Thm Bass'
   // Program Number: 38 'Slap Bass 2'
   {5, 38,0},  //-- 'Slap Bass 2'
   {5, 38,43},  //-- 'Velocity Slap Bass'
   // Program Number: 39 'Syn Bass 1'
   {5, 39,0},  //-- 'Syn Bass 1'
   {5, 39,18},  //-- 'Syn Bass 1 Dark'
   {5, 39,20},  //-- 'Fast Res Bass'
   {5, 39,24},  //-- 'Acid Bass'
   {5, 39,35},  //-- 'Clv Bass'
   {5, 39,40},  //-- 'Tekno Bass'
   {5, 39,64},  //-- 'Oscar Bass'
   {5, 39,65},  //-- 'Sqr Bass'
   {5, 39,66},  //-- 'Rubber Bass'
   {5, 39,96},  //-- 'Hammer Bass'
   // Program Number: 40 'Syn Bass 2'
   {5, 40,0},  //-- 'Syn Bass 2'
   {5, 40,6},  //-- 'Mello S Bass 1'
   {5, 40,12},  //-- 'Seq Bass'
   {5, 40,18},  //-- 'Clk Syn Bass'
   {5, 40,19},  //-- 'Syn Bass 2 Dark'
   {5, 40,32},  //-- 'Smooth Bass 2'
   {5, 40,40},  //-- 'Modular Bass'
   {5, 40,41},  //-- 'DX Bass'
   {5, 40,64},  //-- 'X Wire Bass'

   //--------- Strings group --------------------------
   // Program Number: 41 'Violin'
   {6, 41,0},  //-- 'Violin'
   {6, 41,8},  //-- 'Slow Violin'
   // Program Number: 42 'Viloa'
   {6, 42,0},  //-- 'Viola'
   // Program Number: 43 'Cello'
   {6, 43,0},  //-- 'Cello'
   // Program Number: 44 'Contrabass'
   {6, 44,0},  //-- 'Contrabass'
   // Program Number: 45 'Tremolo String'
   {6, 45,0},  //-- 'Tremolo String'
   {6, 45,8},  //-- 'Slow Trem String'
   {6, 45,40},  //-- 'Susp String'
   // Program Number: 46 'Pizzicatto String'
   {6, 46,0},  //-- 'Pizzicatto String'
   // Program Number: 47 'Harp'
   {6, 47,0},  //-- 'Harp'
   {6, 47,40},  //-- 'Yang Chin String'
   // Program Number: 48 'Timpani'
   {6, 48,0},  //-- 'Timpani'

   //--------- Ensemble group --------------------------
   // Program Number: 49 'String Ensemble 1'
   {7, 49,0},  //-- 'String Ensemble 1'
   {7, 49,3},  //-- 'S. Strings'
   {7, 49,8},  //-- 'Slow Strings'
   {7, 49,24},  //-- 'Arco Strings'
   {7, 49,35},  //-- '60s String Ensemble'
   {7, 49,40},  //-- 'Orchestral'
   {7, 49,41},  //-- 'Orchestral 2'
   {7, 49,42},  //-- 'Trem Orchestral'
   {7, 49,45},  //-- 'Velo Strings'
   // Program Number: 50 'String Ensemble 2'
   {7, 50,0},  //-- 'String Ensemble 2'
   {7, 50,3},  //-- 'S. Slow Strings'
   {7, 50,8},  //-- 'Legato Strings'
   {7, 50,40},  //-- 'Warm Strings'
   {7, 50,41},  //-- 'Kingdom Strings'
   {7, 50,64},  //-- '70s Strings'
   {7, 50,65},  //-- 'String Ensemble 3'
   // Program Number: 51 'Syn Strings 1'
   {7, 51,0},  //-- 'Syn Strings 1'
   {7, 51,27},  //-- 'Resonant Strings'
   {7, 51,64},  //-- 'Syn Strings 4'
   {7, 51,65},  //-- 'SS Strings'
   // Program Number: 52 'Syn Strings 2'
   {7, 52,0},  //-- 'Syn Strings 2'
   // Program Number: 53 'Choir Aahs'
   {7, 53,0},  //-- 'Choir Aahs'
   {7, 53,3},  //-- 'S. Choir'
   {7, 53,16},  //-- 'Choir Aahs 2'
   {7, 53,32},  //-- 'Melodic Choir'
   {7, 53,40},  //-- 'Choir Strings'
   // Program Number: 54 'Voice Oohs'
   {7, 54,0},  //-- 'Voice Oohs'
   // Program Number: 55 'Syn Voice'
   {7, 55,0},  //-- 'Syn Voice'
   {7, 55,40},  //-- 'Sync Voice 2'
   {7, 55,41},  //-- 'Choral'
   {7, 55,64},  //-- 'Analog Voice'
   // Program Number: 56 'Orchestral Hit'
   {7, 56,0},  //-- 'Orchestral Hit'
   {7, 56,35},  //-- 'Orchestral Hit 2'
   {7, 56,64},  //-- 'Impact'

   //--------- Brass group --------------------------
   // Program Number: 57 'Trumpet'
   {8, 57,0},  //-- 'Trumpet'
   {8, 57,16},  //-- 'Trumpet 2'
   {8, 57,17},  //-- 'Brite Trumpet'
   {8, 57,32},  //-- 'Warm Trumpet'
   // Program Number: 58 'Trombone'
   {8, 58,0},  //-- 'Trombone'
   {8, 58,18},  //-- 'Trombone 2'
   // Program Number: 59 'Tuba'
   {8, 59,0},  //-- 'Tuba'
   {8, 59,16},  //-- 'Tuba 2'
   // Program Number: 60 'Mute Trumpet'
   {8, 60,0},  //-- 'Mute Trumpet'
   // Program Number: 61 'French Horn'
   {8, 61,0},  //-- 'French Horn'
   {8, 61,6},  //-- 'French Horn Solo'
   {8, 61,32},  //-- 'French Horn 2'
   {8, 61,37},  //-- 'Horn Orchestra'
   // Program Number: 62 'Brass Section'
   {8, 62,0},  //-- 'Brass Section'
   {8, 62,35},  //-- 'Trumpet & Trombone'
   {8, 62,40},  //-- 'Brass Section 2'
   {8, 62,41},  //-- 'Hi Brass'
   {8, 62,42},  //-- 'Mello Brass'
   // Program Number: 63 'Syn Brass 1'
   {8, 63,0},  //-- 'Syn Brass 1'
   {8, 63,12},  //-- 'Quack Brass'
   {8, 63,20},  //-- 'Rez Syn Brass'
   {8, 63,24},  //-- 'Poly Brass'
   {8, 63,27},  //-- 'Syn Brass 3'
   {8, 63,32},  //-- 'Jump Brass'
   {8, 63,45},  //-- 'Analog Vel Brass'
   {8, 63,64},  //-- 'Analog Brass 1'
   // Program Number: 64 'Syn Brass 2'
   {8, 64,0},  //-- 'Syn Brass 2'
   {8, 64,18},  //-- 'Soft Brass'
   {8, 64,40},  //-- 'Syn Brass 4'
   {8, 64,41},  //-- 'Chorus Brass'
   {8, 64,45},  //-- 'Vel Brass 2'
   {8, 64,64},  //-- 'Analog Brass 1'

   //--------- Reed group --------------------------
   // Program Number: 65 'Soprano Sax'
   {9, 65,0},  //-- 'Soprano Sax'
   // Program Number: 66 'Alto Sax'
   {9, 66,0},  //-- 'Alto Sax'
   {9, 66,40},  //-- 'Sax Section'
   {9, 66,43},  //-- 'Hypr Alto Sax'
   // Program Number: 67 'Tenor Sax'
   {9, 67,0},  //-- 'Tenor Sax'
   {9, 67,40},  //-- 'Bright Tenor Sax'
   {9, 67,41},  //-- 'Soft Tenor Sax'
   {9, 67,64},  //-- 'Tenor Sax 2'
   // Program Number: 68 'Baritone Sax'
   {9, 68,0},  //-- 'Baritone Sax'
   // Program Number: 69 'Oboe'
   {9, 69,0},  //-- 'Oboe'
   // Program Number: 70 'English Horn'
   {9, 70,0},  //-- 'English Horn'
   // Program Number: 71 'Bassoon'
   {9, 71,0},  //-- 'Bassoon'
   // Program Number: 72 'Clarient'
   {9, 72,0},  //-- 'Clarient'

   //--------- Pipe group --------------------------
   // Program Number: 73 'Piccolo'
   {10, 73,0},  //-- 'Piccolo'
   // Program Number: 74 'Flute'
   {10, 74,0},  //-- 'Flute'
   // Program Number: 75 'Recorder'
   {10, 75,0},  //-- 'Recorder'
   // Program Number: 76 'Pan Flute'
   {10, 76,0},  //-- 'Pan Flute'
   // Program Number: 77 'Bottle'
   {10, 77,0},  //-- 'Bottle'
   // Program Number: 78 'Shakuhachi'
   {10, 78,0},  //-- 'Shakuhachi'
   // Program Number: 79 'Whistle'
   {10, 79,0},  //-- 'Whistle'
   // Program Number: 80 'Ocarina'
   {10, 80,0},  //-- 'Ocarina'

   //--------- Synth Lead group --------------------------
   // Program Number: 81 'Square Lead'
   {11, 81,0},  //-- 'Square Lead'
   {11, 81,6},  //-- 'Square Lead 2'
   {11, 81,8},  //-- 'LMSquare'
   {11, 81,18},  //-- 'Hollow'
   {11, 81,19},  //-- 'Shmoog'
   {11, 81,64},  //-- 'Mellow'
   {11, 81,65},  //-- 'SoloSine'
   {11, 81,66},  //-- 'Sine Lead'
   // Program Number: 82 'Sawtooth Lead'
   {11, 82,0},  //-- 'Sawtooth Lead'
   {11, 82,6},  //-- 'Sawtooth 2'
   {11, 82,8},  //-- 'Thich Saw '
   {11, 82,18},  //-- 'Dyna Saw'
   {11, 82,19},  //-- 'Digi Saw'
   {11, 82,20},  //-- 'Big Lead'
   {11, 82,24},  //-- 'Heavy Synth'
   {11, 82,25},  //-- 'Wasphy Synth'
   {11, 82,40},  //-- 'Pulse Saw'
   {11, 82,41},  //-- 'Dr. Lead'
   {11, 82,45},  //-- 'Velo Lead Synth'
   {11, 82,96},  //-- 'Seq Analog Synth'
   // Program Number: 83 'Caliope Lead'
   {11, 83,0},  //-- 'Caliope Lead'
   {11, 83,65},  //-- 'Pure Pad'
   // Program Number: 84 'Chiff Lead'
   {11, 84,0},  //-- 'Chiff Lead'
   {11, 84,64},  //-- 'Rubby'
   // Program Number: 85 'Charan Lead'
   {11, 85,0},  //-- 'Charan Lead'
   {11, 85,64},  //-- 'Distortion Lead'
   {11, 85,65},  //-- 'Wire Lead'
   // Program Number: 86 'Voice Lead'
   {11, 86,0},  //-- 'Voice Lead'
   {11, 86,24},  //-- 'Synth Aah'
   {11, 86,64},  //-- 'Vox Lead'
   // Program Number: 87 'Fifth Lead'
   {11, 87,0},  //-- 'Fifth Lead'
   {11, 87,35},  //-- 'Big Five'
   // Program Number: 88 'Bass & Lead'
   {11, 88,0},  //-- 'Bass & Lead'
   {11, 88,16},  //-- 'Big & Low'
   {11, 88,64},  //-- 'Fat & Porky'
   {11, 88,65},  //-- 'Soft Wurl'

   //--------- Synth Pad group --------------------------
   // Program Number: 89 'New Age Pad'
   {12, 89,0},  //-- 'New Age Pad'
   {12, 89,64},  //-- 'Fantasy 2'
   // Program Number: 90 'Warm Padf'
   {12, 90,0},  //-- 'Warm Padf'
   {12, 90,16},  //-- 'Thick Pad'
   {12, 90,17},  //-- 'Soft Pad'
   {12, 90,18},  //-- 'Sine Pad'
   {12, 90,64},  //-- 'Horn Pad'
   {12, 90,65},  //-- 'Rotary String'
   // Program Number: 91 'Poly Synth Pad'
   {12, 91,0},  //-- 'Poly Synth Pad'
   {12, 91,64},  //-- 'Poly Pad 80'
   {12, 91,65},  //-- 'Click Pad'
   {12, 91,66},  //-- 'Analog Pad'
   {12, 91,67},  //-- 'Square Pad'
   // Program Number: 92 'Choir Pad'
   {12, 92,0},  //-- 'Choir Pad'
   {12, 92,64},  //-- 'Heaven 2'
   {12, 92,66},  //-- 'Itopia'
   {12, 92,67},  //-- 'CC Pad'
   // Program Number: 93 'Bowed Pad'
   {12, 93,0},  //-- 'Bowed Pad'
   {12, 93,64},  //-- 'Glacier'
   {12, 93,65},  //-- 'Glass Pad'
   // Program Number: 94 ' Metal Pad'
   {12, 94,0},  //-- ' Metal Pad'
   {12, 94,64},  //-- 'Tine Pad'
   {12, 94,65},  //-- 'Pan Pad'
   // Program Number: 95 'Halo Pad'
   {12, 95,0},  //-- 'Halo Pad'
   // Program Number: 96 'Sweep Pad'
   {12, 96,0},  //-- 'Sweep Pad'
   {12, 96,20},  //-- 'Shwimmer'
   {12, 96,27},  //-- 'Converge'
   {12, 96,64},  //-- 'Polar Pad'
   {12, 96,66},  //-- 'Celestrial'

   //--------- Synth Effects group --------------------------
   // Program Number: 97 'Rain'
   {13, 97,0},  //-- 'Rain'
   {13, 97,45},  //-- 'Clavi Pad'
   {13, 97,64},  //-- 'Hrmo Rain'
   {13, 97,65},  //-- 'African Wind'
   {13, 97,66},  //-- 'Caribbean'
   // Program Number: 98 'Sound Track'
   {13, 98,0},  //-- 'Sound Track'
   {13, 98,27},  //-- 'Prologue'
   {13, 98,64},  //-- 'Ancestral'
   // Program Number: 99 'Crystal'
   {13, 99,0},  //-- 'Crystal'
   {13, 99,12},  //-- 'Syn Dr Cmp'
   {13, 99,14},  //-- 'Popcorn'
   {13, 99,18},  //-- 'Tiny Bell'
   {13, 99,35},  //-- 'Round Glock'
   {13, 99,40},  //-- 'Clock Chi'
   {13, 99,41},  //-- 'Clear Bell'
   {13, 99,42},  //-- 'Chorus Bell'
   {13, 99,64},  //-- 'Syn Malet'
   {13, 99,65},  //-- 'Soft Crystal'
   {13, 99,66},  //-- 'Loud Glock'
   {13, 99,67},  //-- 'Xmas Bell'
   {13, 99,68},  //-- 'Vibe Bell'
   {13, 99,69},  //-- 'Digi Bell'
   {13, 99,70},  //-- 'Air Bells'
   {13, 99,71},  //-- 'Bell Harp'
   {13, 99,72},  //-- 'Gamelamba'
   // Program Number: 100 'Atmosphere'
   {13, 100,0},  //-- 'Atmosphere'
   {13, 100,18},  //-- 'Warm Atmosphere'
   {13, 100,19},  //-- 'Hollow Rls'
   {13, 100,40},  //-- 'Nylon EP'
   {13, 100,64},  //-- 'Nylon Harp'
   {13, 100,65},  //-- 'Harp Vox'
   {13, 100,66},  //-- 'Atmosphere Pad'
   {13, 100,67},  //-- 'Planet'
   // Program Number: 101 'Bright'
   {13, 101,0},  //-- 'Bright'
   {13, 101,64},  //-- 'Fantasy Bell'
   {13, 101,96},  //-- 'Smokey'
   // Program Number: 102 'Goblins'
   {13, 102,0},  //-- 'Goblins'
   {13, 102,64},  //-- 'Goblin Syn'
   {13, 102,65},  //-- '50s SciFi'
   {13, 102,66},  //-- 'Ring Pad'
   {13, 102,67},  //-- 'Ritual'
   {13, 102,68},  //-- 'To Heaven'
   {13, 102,70},  //-- 'Night'
   {13, 102,71},  //-- 'Glisten'
   {13, 102,96},  //-- 'Bell Choir'
   // Program Number: 103 'Echoes'
   {13, 103,0},  //-- 'Echoes'
   {13, 103,8},  //-- 'Echo Pad 2'
   {13, 103,14},  //-- 'Echo Pan'
   {13, 103,64},  //-- 'Echo Bell'
   {13, 103,65},  //-- 'Big Pan'
   {13, 103,66},  //-- 'Syn Piano'
   {13, 103,67},  //-- 'Creation'
   {13, 103,68},  //-- 'Stardust'
   {13, 103,69},  //-- 'Resonant Pan'
   // Program Number: 104 'Sci-Fi'
   {13, 104,0},  //-- 'Sci-Fi'
   {13, 104,64},  //-- 'Starz'

   //--------- Ethnic group --------------------------
   // Program Number: 105 'Sitar'
   {14, 105,0},  //-- 'Sitar'
   {14, 105,32},  //-- 'Det Sitar'
   {14, 105,35},  //-- 'Sitar 2'
   {14, 105,96},  //-- 'Tambra'
   {14, 105,97},  //-- 'Tamboura'
   // Program Number: 106 'Banjo'
   {14, 106,0},  //-- 'Banjo'
   {14, 106,28},  //-- 'Mute Banjo'
   {14, 106,96},  //-- 'Rabab'
   {14, 106,97},  //-- 'Gopichant'
   {14, 106,98},  //-- 'Oud'
   // Program Number: 107 'Shamisen'
   {14, 107,0},  //-- 'Shamisen'
   // Program Number: 108 'Koto'
   {14, 108,0},  //-- 'Koto'
   {14, 108,96},  //-- 'T. Koto'
   {14, 108,97},  //-- 'Kanoon'
   // Program Number: 109 'Kalimba'
   {14, 109,0},  //-- 'Kalimba'
   // Program Number: 110 'Bagpipe'
   {14, 110,0},  //-- 'Bagpipe'
   // Program Number: 111 'Fiddle'
   {14, 111,0},  //-- 'Fiddle'
   // Program Number: 112 'Shanai'
   {14, 112,0},  //-- 'Shanai'
   {14, 112,64},  //-- 'Shanai 2'
   {14, 112,96},  //-- 'Pungi'
   {14, 112,97},  //-- 'Hichriki'

   //--------- Percussive group --------------------------
   // Program Number: 113 'Tinkle Bell'
   {15, 113,0},  //-- 'Tinkle Bell'
   {15, 113,96},  //-- 'Bonang'
   {15, 113,97},  //-- 'Gender'
   {15, 113,98},  //-- 'Gamelan'
   {15, 113,99},  //-- 'S. Gamelan'
   {15, 113,100},  //-- 'Rama Cymbal'
   {15, 113,101},  //-- 'Asian Bell'
   // Program Number: 114 'Agogo'
   {15, 114,0},  //-- 'Agogo'
   // Program Number: 115 'Steel Drum'
   {15, 115,0},  //-- 'Steel Drum'
   {15, 115,97},  //-- 'Glass Percussion'
   {15, 115,98},  //-- 'Thai Bell'
   // Program Number: 116 'Wood Block'
   {15, 116,0},  //-- 'Wood Block'
   {15, 116,96},  //-- 'Castanet'
   // Program Number: 117 'Taiko Drum'
   {15, 117,0},  //-- 'Taiko Drum'
   {15, 117,96},  //-- 'Gr. Cassanet'
   // Program Number: 118 'Melodic Tom'
   {15, 118,0},  //-- 'Melodic Tom'
   {15, 118,64},  //-- 'Melodict Tom 2'
   {15, 118,65},  //-- 'Real Tom'
   {15, 118,66},  //-- 'Rock Tom'
   // Program Number: 119 'Syn Drum'
   {15, 119,0},  //-- 'Syn Drum'
   {15, 119,64},  //-- 'Analog Tom'
   {15, 119,65},  //-- 'Electric Percussion'
   // Program Number: 120 'Reverse Cymbal'
   {15, 120,0},  //-- 'Reverse Cymbal'

   //--------- Sound Effects group --------------------------
   // Program Number: 121 'Guitar Fret Noise'
   {16, 121,0},  //-- 'Guitar Fret Noise'
   // Program Number: 122 'Breath Noise'
   {16, 122,0},  //-- 'Breath Noise'
   // Program Number: 123 'Seashore'
   {16, 123,0},  //-- 'Seashore'
   // Program Number: 124 'Bird Tweet'
   {16, 124,0},  //-- 'Bird Tweet'
   // Program Number: 125 'Telephone'
   {16, 125,0},  //-- 'Telephone'
   // Program Number: 126 'Helicopter'
   {16, 126,0},  //-- 'Helicopter'
   // Program Number: 127 'Applause'
   {16, 127,0},  //-- 'Applause'
   // Program Number: 128 'Gunshot'
   {16, 128,0},  //-- 'Gunshot'
};

// Array of indexes to the first voice for each ProgramNumber set in Disklavier
const int disklavierProgramNumberIndex[] = {
0,5,7,12,14,21,30,34,39,40,41,43,46,51,52,55,59,72,77,81,87,
89,91,93,95,100,106,109,111,116,118,121,124,127,134,136,142,145,147,157,166,
168,169,170,171,174,175,177,178,187,194,198,199,204,205,209,212,216,218,220,221,
225,230,238,244,245,248,252,253,254,255,256,257,258,259,260,261,262,263,264,265,
273,285,287,289,292,295,297,301,303,309,314,318,321,324,325,330,335,338,355,363,
366,375,384,386,391,396,397,400,401,402,403,407,414,415,418,420,422,426,429,430,
431,432,433,434,435,436,437 };

// Array of indexes to the start of each Disklavier Group.
const int disklavierGroupIndex[NUM_DISKLAVIER_GROUPS] = {0,39,59,95,124,166,178,212,244,257,265,301,330,386,407,430 };

// Disklavier voice names with same index order as disklavierVoiceMIDINumbers
const char* disklavierVoiceNames[NUM_DISKLAVIER_VOICES] = {
"Grand Piano","Grand Piano K","Mello Grand Piano","Piano Strings","Piano Dream","Brite Piano",
"Brite Piano K","Elec Grand Piano","Elec Grand Piano K","Det. CP80","Elec Grand Piano 1",
"Elec Grand Piano 2","Honky tonk Piano 2","Honky Tonk Piano K 2","Elec Piano 1","Elec Piano K 1",
"Mello Elec Piano 1","Chorus Elec Piano 1","Hard Elec Piano 1","VX Elec Piano 1","60s Elec Pianao",
"Elec Piano 2","Elec Piano 2 K","Chorus Elec Piano 2","DX Hard","DX Legend",
"DX Phase","DX+Analog","DX Koto Elec Piano","VX Elec Piano 2","Harpsichord",
"Harpsichord K","Harpsichord 2","Harpsichord 3","Clavinet","Clavinet K",
"Clavinet Wah","Pulse Clavinet","Pierce Clavinet","Celesta","Glocken",
"Music Box","Orgel","Vibes","Vibes K","Hard Vibe",
"Marimba","Marimba K","Sine Marimba","Balafone","Log Drum",
"Xylophone","Tubular Bell","Church Bell","Carillon","Dulcimer",
"Dulcimer 2","Cimbalom","Santur","Drawbar Organ","Det Draw Organ",
"60s Draw Organ 1","60s Draw Organ 2","70s Draw Organ 1"," Drawbar Organ 2","60s Draw Organ 3",
"Even Bar Organ","16+2\"2/3 Organ","Organ Bar","70s Draw Organ 2","Cheeze Organ",
"Draw Organ 3","PercussionOrgan","70s Perc Organ","Det Perc Organ","Lite Organ",
"Percussion Organ 2","Rock Organ","Rotary Organ","Slow Rotary Organ","Fast Rotary Organ",
"Church Organ","Church Organ 3","Church Organ 2","Notre Dame Organ","Organ Flute",
"Trm Organ Flute","Reed Organ","Puff Organ","Accordion","Accodian Light",
"Harmonica","Harmonica 2","Tango Accordian","Tango Accordian 2","Nylon Guitar",
"Nylon Guitar 2","Nylon Guitar 3","Vel Guitar Harmonic","Ukulele","Steel Guitar",
"Steel Guitar 2","12 String Guitar","Nylon & Steel Guitar","Steel & Body Guitar","Mandolin",
"Jazz Guitar","Mello Guitar","Jazz Amp Guitar","Clean Guitar","Chorus Guitar",
"Mute Guitar","Funk Guitar","Mute Steel Guitar","Funk Guitar 2","Jazz Man Guitar",
"Overdrive Guitar","Guitar Pinch","Distortion Guitar","Feedback Guitar","Feedback Guitar 2",
"Guitar Harmonics","Guitar Feedback","Guitar Harmonics 2","Acoustic Bass","Jazz Rhythm Bass",
"VX Upright Bass","Fingered Bass","Fingered Dark Bass","Flange Bass","Bass & Dist Elec Guitar",
"Finger Slap Bass","Fingered Bass 2","Mod Alem Bass","Picked Bass","Mute Picked Bass",
"Fretless Bass","Fretless Bass 2","Fretless Bass 3","Fretless Bass 4","Syn Fretfless Bass",
"Smooth Bass","Slap Bass","Resonant Slap Bass","Punch Thm Bass","Slap Bass 2",
"Velocity Slap Bass","Syn Bass 1","Syn Bass 1 Dark","Fast Res Bass","Acid Bass",
"Clv Bass","Tekno Bass","Oscar Bass","Sqr Bass","Rubber Bass",
"Hammer Bass","Syn Bass 2","Mello S Bass 1","Seq Bass","Clk Syn Bass",
"Syn Bass 2 Dark","Smooth Bass 2","Modular Bass","DX Bass","X Wire Bass",
"Violin","Slow Violin","Viola","Cello","Contrabass",
"Tremolo String","Slow Trem String","Susp String","Pizzicatto String","Harp",
"Yang Chin String","Timpani","String Ensemble 1","S. Strings","Slow Strings",
"Arco Strings","60s String Ensemble","Orchestral","Orchestral 2","Trem Orchestral",
"Velo Strings","String Ensemble 2","S. Slow Strings","Legato Strings","Warm Strings",
"Kingdom Strings","70s Strings","String Ensemble 3","Syn Strings 1","Resonant Strings",
"Syn Strings 4","SS Strings","Syn Strings 2","Choir Aahs","S. Choir",
"Choir Aahs 2","Melodic Choir","Choir Strings","Voice Oohs","Syn Voice",
"Sync Voice 2","Choral","Analog Voice","Orchestral Hit","Orchestral Hit 2",
"Impact","Trumpet","Trumpet 2","Brite Trumpet","Warm Trumpet",
"Trombone","Trombone 2","Tuba","Tuba 2","Mute Trumpet",
"French Horn","French Horn Solo","French Horn 2","Horn Orchestra","Brass Section",
"Trumpet & Trombone","Brass Section 2","Hi Brass","Mello Brass","Syn Brass 1",
"Quack Brass","Rez Syn Brass","Poly Brass","Syn Brass 3","Jump Brass",
"Analog Vel Brass","Analog Brass 1","Syn Brass 2","Soft Brass","Syn Brass 4",
"Chorus Brass","Vel Brass 2","Analog Brass 1","Soprano Sax","Alto Sax",
"Sax Section","Hypr Alto Sax","Tenor Sax","Bright Tenor Sax","Soft Tenor Sax",
"Tenor Sax 2","Baritone Sax","Oboe","English Horn","Bassoon",
"Clarient","Piccolo","Flute","Recorder","Pan Flute",
"Bottle","Shakuhachi","Whistle","Ocarina","Square Lead",
"Square Lead 2","LMSquare","Hollow","Shmoog","Mellow",
"SoloSine","Sine Lead","Sawtooth Lead","Sawtooth 2","Thich Saw ",
"Dyna Saw","Digi Saw","Big Lead","Heavy Synth","Wasphy Synth",
"Pulse Saw","Dr. Lead","Velo Lead Synth","Seq Analog Synth","Caliope Lead",
"Pure Pad","Chiff Lead","Rubby","Charan Lead","Distortion Lead",
"Wire Lead","Voice Lead","Synth Aah","Vox Lead","Fifth Lead",
"Big Five","Bass & Lead","Big & Low","Fat & Porky","Soft Wurl",
"New Age Pad","Fantasy 2","Warm Padf","Thick Pad","Soft Pad",
"Sine Pad","Horn Pad","Rotary String","Poly Synth Pad","Poly Pad 80",
"Click Pad","Analog Pad","Square Pad","Choir Pad","Heaven 2",
"Itopia","CC Pad","Bowed Pad","Glacier","Glass Pad",
" Metal Pad","Tine Pad","Pan Pad","Halo Pad","Sweep Pad",
"Shwimmer","Converge","Polar Pad","Celestrial","Rain",
"Clavi Pad","Hrmo Rain","African Wind","Caribbean","Sound Track",
"Prologue","Ancestral","Crystal","Syn Dr Cmp","Popcorn",
"Tiny Bell","Round Glock","Clock Chi","Clear Bell","Chorus Bell",
"Syn Malet","Soft Crystal","Loud Glock","Xmas Bell","Vibe Bell",
"Digi Bell","Air Bells","Bell Harp","Gamelamba","Atmosphere",
"Warm Atmosphere","Hollow Rls","Nylon EP","Nylon Harp","Harp Vox",
"Atmosphere Pad","Planet","Bright","Fantasy Bell","Smokey",
"Goblins","Goblin Syn","50s SciFi","Ring Pad","Ritual",
"To Heaven","Night","Glisten","Bell Choir","Echoes",
"Echo Pad 2","Echo Pan","Echo Bell","Big Pan","Syn Piano",
"Creation","Stardust","Resonant Pan","Sci-Fi","Starz",
"Sitar","Det Sitar","Sitar 2","Tambra","Tamboura",
"Banjo","Mute Banjo","Rabab","Gopichant","Oud",
"Shamisen","Koto","T. Koto","Kanoon","Kalimba",
"Bagpipe","Fiddle","Shanai","Shanai 2","Pungi",
"Hichriki","Tinkle Bell","Bonang","Gender","Gamelan",
"S. Gamelan","Rama Cymbal","Asian Bell","Agogo","Steel Drum",
"Glass Percussion","Thai Bell","Wood Block","Castanet","Taiko Drum",
"Gr. Cassanet","Melodic Tom","Melodict Tom 2","Real Tom","Rock Tom",
"Syn Drum","Analog Tom","Electric Percussion","Reverse Cymbal","Guitar Fret Noise",
"Breath Noise","Seashore","Bird Tweet","Telephone","Helicopter",
"Applause","Gunshot", };

//------------- End of Disklavier MIDI patch
