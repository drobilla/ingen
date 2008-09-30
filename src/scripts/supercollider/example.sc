// Boot Ingen (inside SC) and the scsynth server
(
o=Ingen.boot {
	o.loadPatch("/home/mlang/src/om-synth/src/clients/patches/303.om");
	s.boot;
}
)
// Connect patch output to one SC input
o.jackConnect("/303/output", "SuperCollider:in_3");
// Play this input on the default output bus (most simple postprocessor)
{AudioIn.ar(3).dup}.play;
// A simple 303 pattern
(
Tempo.bpm=170;
Ppar([
	Pbind(\type, \om, \target, o.getPatch("/303",true),
		\dur, 1/1, \octave,2,\degree, Pseq([Pshuf([0,2,4,6],16)],2)),
	Pbind(\type, \om, \target, o.getPatch("/303",true),
		\dur, Prand([Pshuf([1/2,1/4,1/4],4),Pshuf([1/3,1/3,1/6,1/6],4)],inf),
		\legato, Prand((0.5,0.55..0.9),inf),
		\octave,3,\degree, Pseq([Pshuf([0,2,4,6],2)],40)),
	Pbind(\type, \om, \omcmd, \setPortValue,
		\target, o.getPort("/303/cutoff",true),
		\dur, 1/10, \portValue, Pseq((0,0.005..1).mirror,2)),
	Pbind(\type, \om, \omcmd, \setPortValue,
		\target, o.getPort("/303/resonance",true), \portValue, 1)]).play(quant:4)
)
