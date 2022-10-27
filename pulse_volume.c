/*
	___pulseaudio volume feedback module for slstatus___

A HUGE portion of the knowledge that was necessary for me to figure out
how to write this piece of software can be found in this repository:
___ https://github.com/cdemoulins/pamixer ______
He's using an object oriented approach, I'm not

This is what inspired the structure of the logic:
http://www.freedesktop.org/wiki/Software/PulseAudio/Documentation/Developer/Clients/Samples/AsyncDeviceList/

this last one is hiding in plain sight; google doesn't like open source audio backends
*/

#ifdef DEBUG
	#include <stdio.h>
#endif

#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>

#include "../util.h"

typedef enum {
	CONNECTING,
	CONNECTED,
	ERROR
} state_t;

void pa_state_cb(pa_context *ctx, void *userdata);
void pa_serverinfo_cb(pa_context *ctx, const pa_server_info *pa_i, void *userdata);
void pa_sinkvol_cb(pa_context *ctx, const pa_sink_info *pa_i, int eol, void *userdata);
char *get_arg_char(const char *chargs, int index);

#ifdef DEBUG
char buf[1024];
#endif

#define CONNERR "PulseAudio connection error"
#define MLERR  "PulseAudio mainloop error"

#define PA_DISCONNECT do { 				\
			pa_context_disconnect(pa_ctx);  \
            		pa_context_unref(pa_ctx);       \
            		pa_mainloop_free(pa_ml);	\
		      } while(0)

const char*
pulse_volume(const char *chargs){

	char default_sink_name[512];

	int process_step = 0;
	int sinkvol;

	pa_mainloop *pa_ml;
	pa_mainloop_api *pa_mlapi;
	pa_operation *pa_op;
	pa_context *pa_ctx;

	state_t pa_ready = CONNECTING;

	pa_ml = pa_mainloop_new();
	pa_mlapi = pa_mainloop_get_api(pa_ml);
	pa_ctx = pa_context_new(pa_mlapi, "slstatus");

	if (pa_context_connect(pa_ctx, NULL, 0, NULL)<0){
		warn(CONNERR);
		return "";
	}

	pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

	for (;;){
		if (pa_ready == CONNECTING){
			if (pa_mainloop_iterate(pa_ml, 1, NULL)<0){
				PA_DISCONNECT;
				warn(MLERR);
				return "";
			}
			continue;
		}
		if (pa_ready == ERROR) {
			PA_DISCONNECT;
			warn(CONNERR);
			return "";
		}

		switch (process_step) {
		case 0:
			pa_op = pa_context_get_server_info(
					pa_ctx,
					pa_serverinfo_cb,
					default_sink_name);
			process_step++;
			break;
		case 1:
			if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE){
				pa_operation_unref(pa_op);

				pa_op = pa_context_get_sink_info_by_name(
						pa_ctx,
						default_sink_name,
						pa_sinkvol_cb,
						&sinkvol);
				process_step++;
			}
			break;
		case 2:
			if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE) {
				pa_operation_unref(pa_op);
				PA_DISCONNECT;
				if (sinkvol == -1)
					return bprintf(get_arg_char(chargs, 1));
				else
					return bprintf("%s%d%%", get_arg_char(chargs, 0), sinkvol);
			}
		default:
		break;
		}

		pa_mainloop_iterate(pa_ml, 1, NULL);

	}

}

#ifdef DEBUG
int
main(){
	printf("%s", pulse_volume("🔊,🔇"));
	return 0;
}
#endif

// we need this function because I want
// to let the user use characters bigger
// than the size of 'char'
char *
get_arg_char(const char *chargs, int index){
	char *chargs_cpy = alloca(strlen(chargs));
	strcpy(chargs_cpy, chargs);
	char *token = strtok(chargs_cpy, ",");
	return index ? strtok(NULL, ",") : token;
}

void
pa_state_cb(pa_context *ctx, void *userdata){

	state_t *connection_state = userdata;

	switch (pa_context_get_state(ctx)){
		case PA_CONTEXT_READY:
			*connection_state = CONNECTED;
			break;
		case PA_CONTEXT_FAILED:
			*connection_state = ERROR;
			break;
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_TERMINATED:
		default:
		break;
	}
}

void
pa_serverinfo_cb(pa_context *ctx, const pa_server_info *pa_i, void *userdata){
	char *sinkname = userdata;
	strncpy(sinkname, pa_i->default_sink_name, 511);
}

void
pa_sinkvol_cb(pa_context *ctx, const pa_sink_info *pa_i, int eol, void *userdata){
	if (eol != 0) return;
	int *volpercent = userdata;
	if (pa_i->mute){
		*volpercent = -1;
		return;
	}
	pa_cvolume vol = pa_i->volume;
	*volpercent = (int) round( (double)pa_cvolume_avg(&vol) * 100. / PA_VOLUME_NORM );
}
