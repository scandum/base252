/*

Copyright (c)  2020  Igor van den Hoven  ivdhoven@gmail.com

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

*/

/*
	Compile using: gcc <filename> -lz

	The data_to_base252 function uses zlib to compress data, next translates the data
	to Base252 to create valid C strings.

	The base252_to_data function reverses the process.

*/

#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>
#include <string.h>

#define BUFFER_SIZE 10000

/*
	zlib utility functions
*/

void *zlib_alloc( void *opaque, unsigned int items, unsigned int size )
{
	return calloc(items, size);
}


void zlib_free( void *opaque, void *address )
{
	free(address);
}

int zlib_compress(char *in, size_t size_in, char *out, size_t size_out)
{
	char *buf, *pto;
	z_stream *stream;
	int len, cnt;

	stream = calloc(1, sizeof(z_stream));

	stream->next_in     = (unsigned char *) in;
	stream->avail_in    = size_in;

	stream->next_out    = (unsigned char *) out;
	stream->avail_out   = size_out - 1;

	stream->data_type   = Z_ASCII;
	stream->zalloc      = zlib_alloc;
	stream->zfree       = zlib_free;
	stream->opaque      = Z_NULL;

	if (deflateInit(stream, Z_BEST_COMPRESSION) != Z_OK)
	{
		printf("zlib_compress: failed deflateInit2\n");

		free(stream);

		return -1;
	}

	if (deflate(stream, Z_FINISH) != Z_STREAM_END)
	{
		printf("zlib_compress: failed deflate\n");

		free(stream);

		return -1;
	}

	if (deflateEnd(stream) != Z_OK)
	{
		printf("zlib_compress: failed deflateEnd\n");

		free(stream);

		return -1;
	}

	len = size_out - stream->avail_out;

	free(stream);

	return len;
}

int zlib_decompress(char *in, size_t size_in, char *out, size_t size_out)
{
	z_stream *stream;
	int len;

	stream = calloc(1, sizeof(z_stream));

	stream->data_type   = Z_ASCII;
	stream->zalloc      = zlib_alloc;
	stream->zfree       = zlib_free;
	stream->opaque      = Z_NULL;

	if (inflateInit(stream) != Z_OK)
	{
		printf("zlib_decompresss: failed inflateInit\n");

		free(stream);

		return -1;
	}

	stream->next_in     = (unsigned char *) in;
	stream->avail_in    = size_in;

	stream->next_out    = (unsigned char *) out;
	stream->avail_out   = size_out - 1;

	if (inflate(stream, Z_SYNC_FLUSH) == Z_BUF_ERROR)
	{
		printf("zlib_decompress: inflate Z_BUF_ERROR\n");

		len = -1;
	}
	else
	{
		len = stream->next_out - (unsigned char *) out;

		out[len] = 0;
	}

	inflateEnd(stream);

	free(stream);

	return len;
}

/*
	Base252 utility functions
*/

// zlib compress data, next convert data to base252
// returns size of out, not including null-termination

int data_to_base252(char *in, size_t size_in, char *out, size_t size_out)
{
	char *buf, *pto;
	int len, cnt;

	buf = malloc(BUFFER_SIZE);

	len = zlib_compress(in, size_in, buf, size_out);

	if (len == -1)
	{
		return -1;
	}

	pto = out;

	for (cnt = 0 ; cnt < len ; cnt++)
	{
		if (pto - out >= size_out - 2)
		{
			break;
		}

		switch ((unsigned char) buf[cnt])
		{
			case 0:
			case '"':
				*pto++ = 245;
				*pto++ = 128 + (unsigned char) buf[cnt] % 64;
				break;

			case '\\':
				*pto++ = 246;
				*pto++ = 128 + (unsigned char) buf[cnt] % 64;
				break;

			case 245:
			case 246:
			case 247:
			case 248:
				*pto++ = 248;
				*pto++ = 128 + (unsigned char) buf[cnt] % 64;
				break;

			default:
				*pto++ = buf[cnt];
				break;
		}
	}

	*pto = 0;

	free(buf);

	return pto - out;
}

// unconvert data from base252, next zlib decompress data
// returns size of out, not including null-termination

int base252_to_data(char *in, size_t size_in, char *out, size_t size_out)
{
	char *buf, *ptb, *pti;
	z_stream *stream;
	int val, cnt;

	buf = malloc(size_in);

	ptb = buf;
	cnt = 0;

	while (cnt < size_in)
	{
		switch ((unsigned char) in[cnt])
		{
			default:
				*ptb++ = in[cnt++];
				continue;

			case 245:
				*ptb++ = 0 + (unsigned char) in[++cnt] % 64;
				break;

			case 246:
				*ptb++ = 64 + (unsigned char) in[++cnt] % 64;
				break;

			case 247:
				*ptb++ = 128 + (unsigned char) in[++cnt] % 64;
				break;

			case 248:
				*ptb++ = 192 + (unsigned char) in[++cnt] % 64;
				break;
		}

		if (cnt < size_in)
		{
			cnt++;
		}
	}

	val = zlib_decompress(buf, ptb - buf, out, size_out);

	free(buf);

	return val;
}

/*
	Testing
*/

int main(void)
{
	char input[BUFFER_SIZE], output[BUFFER_SIZE];
	int size;

	strcpy(input, "THE SOFTWARE IS PROVIDED \"AS IS\" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.");

	strcpy(output, "");

	size = data_to_base252(input, strlen(input), output, BUFFER_SIZE);

	printf("input is:\n\n%s\n(%d) \n\noutput is:\n\n%s\n\n", input, strlen(input), output);

	memcpy(input, output, size + 1);

	strcpy(output, "");

	base252_to_data(input, size, output, BUFFER_SIZE);

	printf("input is:\n\n%s\n(%d)\n\noutput is:\n\n%s\n\n", input, size, output);

	return 0;
}
