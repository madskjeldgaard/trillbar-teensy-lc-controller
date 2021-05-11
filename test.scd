MIDIIn.connectAll;
MIDIFunc.trace(false);

a = H14BCC.new(\t1, 0, 32, 1);
a.func_({|val| val.postln });

b = H14BCC.new(\t2, 0, 32, 2);
b.func_({|val| val.postln });
