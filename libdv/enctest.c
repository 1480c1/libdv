/* Test libdv's multi-threading capabilities.
 * Usage:
 *    ./enctest      > gray.dv    encodes 1 frame
 *    ./enctest 40   > gray.dv    encodes 40 frames in parallel
 *    ./enctest 40 x > gray.dv    encodes 40 frames sequentially
 *
 */

#include <stdio.h>
#include <libdv/dv.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define N 100

struct frame_t {
	unsigned char rgb[720*480*4];
	unsigned char dv[120000];
} frame[N];

static void* t(void* arg)
{
	int i = (int)arg;
	fprintf(stderr, "thread %p: start frame %d\n", (void*) pthread_self(), i);
	dv_encoder_t* enc = dv_encoder_new(0, 0, 0);
	enc->isPAL = 0;
	enc->vlc_encode_passes = 3;
	enc->static_qno = 0;
	enc->force_dct = DV_DCT_AUTO;

	struct frame_t* f = &frame[i];
	unsigned char* rgb = f->rgb;
	dv_encode_full_frame(enc, &rgb, e_dv_color_rgb, f->dv);

	dv_encoder_free(enc);
	fprintf(stderr, "thread %p: done frame %d\n", (void*) pthread_self(), i);
	return 0;
}

int main(int argc, char** argv)
{
	pthread_t th[N];
	int i, n = 1, parallel = 1;
	void* r;

	if (argc > 1 && sscanf(argv[1], "%d", &n) == 1)
		n = n <= N ? n > 0 ? n : 1 : N;	/* restrict to 1 .. N */
	if (argc > 2)
		parallel = 0;

	memset(&frame, 100, sizeof(frame));	/* all frames are gray */

	for (i = 0; i < n; i++) {
		if (parallel)
			pthread_create(&th[i], 0, t, (void*)i);
		else
			t((void*)i);
	}

	for (i = 0; i < n; i++) {
		if (parallel)
			pthread_join(th[i], &r);
		write(1, frame[i].dv, sizeof(frame[i].dv));
	}
	
	return 0;
}
