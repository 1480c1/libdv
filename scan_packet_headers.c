#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, const char** argv)
{
	unsigned char buf[80];
	int dif_count = -1;
	int packet = -1;
	int frame = 0;

	while (read(STDIN_FILENO, buf, 80) == 80) {
		int i;
		dif_count++;
		if ((dif_count % 6) == 0) {
			if (packet == 299) {
				packet = -1;
				dif_count = 0;
				frame++;
				printf("------------------------\n");
			}
			packet++;
		}
		printf("%04d/%03d/%03d :", dif_count, packet, frame);
		for (i = 0; i < 40; i++) {
			printf("%02x ", buf[i]);
		}
		printf("| ");
		for (i = 40; i < 80; i++) {
			printf("%02x ", buf[i]);
		}
		printf("\n");
	}

}
