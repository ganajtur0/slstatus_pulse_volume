// TODO: this readme blows.

--- Read the source files ---

But if you don't want to, here's a quick overview of what this thing is:
I copied pamixer and some tutorial code I've found online detailing the usage of PulseAudio.
I've tried to follow the coding conventions of the original repo.
I'm not a professional C developer.
This works for a pipewire as pulseaudio kind of setup as well (that's what I have).

You shouldn't blindly copy the files into your slstatus source, except for the pulse_volume.c file.

--- File locations ---

pretend, that the root (/) is /whereever_you_pulled_down_slstatus/slstatus/

pulse_volume.c -> /components/
config.mk -> /
config.h -> /
slstatus.h -> /

I've marked the sections of the files that I've modified. Hopefully all of'em.
-------------------------------------------------------------
^
|
--these lines are the marks

I'll probably make diff files in the near future.

--- Goodbye ---
