Note the text etc.. on this plugin will likely change in later versions, the whole point of this one was
to port some of Behringers onboard DSP effects from the DDX3216 as I am learning about bare metal programming
so I thought I would see if I could build the basic plugins in juce by making analogous code structures
but then had to refine stuff a little. In fact I learned that JUCE using CLAP plugins can get much closer
to some of the  methods use for sharc due to serialization of multiple processors which is forbidden in
vst3 but these plugins are basic enough that VST3 is just fine.  This never actually reverse engineered
the DDX3216 sourcecode from the firmware for the effects although I am interested in doing that this step
was simply taking some of the basic bare metal plugins and trying to build them in juce.  
I'm not sort of trying to make them more CPU efficent as although they are about half of what they were 
originally   they are still pretty heavy CPU load for a basic tremolo plugin for instance. 
