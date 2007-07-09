#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int PromptYesNo(char *question) {
	int ch, rv;
	printf("%s [y/N] ", question);
	ch = getchar();
	rv = (ch=='y'||ch=='Y');
	while (ch!='\n'&&ch!=EOF) {
		ch = getchar();
	}
	return rv;
}

int ConvertV1toV2(FILE *in, FILE *out) {
	int flags = 0, reset;
	size_t size;

	unsigned char buf[4096];

	if (PromptYesNo("Recorded in PAL mode?")) {
		flags |= 2;
	}

	if (PromptYesNo("Recorded in Japan mode?")) {
		flags |= 4;
	}

	fread(buf, 0x60, 1, in);

	reset = *(int *)(buf + 0x10);

	*(int *)(buf + 0x14) = reset ? 0 : 0x64;
	*(int *)(buf + 0x18) = reset ? 0x64 : 0x64+25088;

	fwrite(buf, 0x60, 1, out);

	fwrite(&flags, 4, 1, out);

	do {
		size = fread(buf, 1, sizeof(buf), in);
		if (size > 0) fwrite(buf, size, 1, out);
	} while (size == sizeof(buf));

	return 1;
}

int InplaceConvert(int (*ConvertFn)(FILE *, FILE *), char *filename) {
	FILE *fd, *fdtmp;
	int filenamelen = strlen(filename);
	char *filenametmp;
	int rv;

	fd = fopen(filename, "rb");
	if (fd == 0) {
		perror("fopen");
		return 0;
	}

	filenametmp = malloc(filenamelen + 5);
	strcpy(filenametmp, filename);
	strcpy(filenametmp+filenamelen, ".tmp");

	fdtmp = fopen(filenametmp, "wb");
	if (fdtmp == 0) {
		perror("fopen");
		free(filenametmp);
		return 0;
	}

	rv = ConvertFn(fd, fdtmp);
	if (rv == 0) {
		free(filenametmp);
		return 0;
	}

	fclose(fd); fclose(fdtmp);

	rv = remove(filename);
	if (rv == -1) {
		perror("remove");
		free(filenametmp);
		return 0;
	}

	rv = rename(filenametmp, filename);
	if (rv == -1) {
		perror("rename");
		free(filenametmp);
		return 0;
	}

	free(filenametmp);
	return 1;
}

int main(int argc, char **argv) {
	FILE *fd;
	int reset, stateOffset, inputOffset, packetSize, headerSize;

	if (argc < 2) {
		printf("Syntax: %s movie.mmv\n", argv[0]);
		return 1;
	}

	fd = fopen(argv[1], "rb");
	if (fd == 0) {
		perror("fopen");
		return 1;
	}

	fseek(fd, 0x10, SEEK_SET);
	fread(&reset, 4, 1, fd);
	fread(&stateOffset, 4, 1, fd);
	fread(&inputOffset, 4, 1, fd);
	fread(&packetSize, 4, 1, fd);
	fclose(fd);

	headerSize = reset ? inputOffset : stateOffset;
	if (headerSize == 0) headerSize = 0x60;

	if (headerSize == 0x60) {
		InplaceConvert(ConvertV1toV2, argv[1]);
	} else if (headerSize == 0x64) {
		puts("File is already latest version.");
		return 1;
	} else {
		puts("Could not identify file version!");
		return 1;
	}

	return 0;
}
