#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/buffer.h>
#include <assert.h>

#include "util.h"
#include "connections.h"
#include "../payload_server.h"

#include "file_steg.h"
#include "jpgSteg.h"

int JPGSteg::modify_huffman_table(uint8_t* raw_data, int len)
{
	int counter = 0;
	for (int i = 0; i < len-1; i++) {
		if (raw_data[i] == FRAME && raw_data[i+1] == FRAME_HUFFMAN) {
			short *s = (short *) (raw_data+i+2);
			unsigned short len2 = SWAP(*s);
			short codes = len2 - 3 - 16;
			LOG("Huffman Table Codes: %hd\n", codes)
			int j;
			for (j = 0; j < codes; j++) {
				raw_data[i+5+j+16] = 1;
			}
			counter++;
		}
	}
	return counter;

}

int JPGSteg::corrupt_reset_interval(uint8_t* raw_data, int len)
{
	int i;
	int counter = 0;
	for (i = 0; i < len-1; i++) {
		if (raw_data[i] == FRAME && raw_data[i+1] == FRAME_RST) {
			short *ri = (short *) (raw_data+i + 2);
			*ri = 0xFFFF;
			counter++;
		}
	}
	return counter;

}

int JPGSteg::starting_point(const uint8_t *raw_data, int len)
{
	int i;
	int lm = 0; // Last Marker
	//long lf; // Last Frame
	for (i = 0; i < len-1; i++) {
		if (raw_data[i] == FRAME && raw_data[i+1] == FRAME_SCAN) {
			lm = i;
			LOG("0xFFDA at %06X\n", i)
		} else if (raw_data[i] == FRAME && raw_data[i+1] == FRAME_RST) {
			LOG("0xFFDD at %06X\n", i)
		} else if (raw_data[i] == FRAME && raw_data[i+1] == FRAME_RST0) {
			LOG("0xFFD0 at %06X\n", i)
		}
	}
	const unsigned short *flen = (const unsigned short *)(raw_data+lm+2); // Frame length
	unsigned short swapped = SWAP(*flen);
	log_info("Size of the last DA frame: %hhu at %06X\n", swapped, lm);

	// TODO: Ignore RSTn bytes (Restart Interval)	
	/*
	long lf2;
	for (i = lm+*flen; i < len-1; i++) {
		if (raw_data[i] == 0xFF && raw_data[i+1] != 0x00) {
			printf("nooo FF%hhX\n", raw_data[i+1]);
			lf2 = i;
			//break;
		}
	}
	*/
	//int c = lf - lm - *flen - 2;

	return lm + 2 + *flen; // 2 for FFDA, and skip the header
}

ssize_t JPGSteg::capacity(const uint8_t *cover_payload, size_t len)
{
  return static_capacity((char*)cover_payload, len);
}

//Temp: should get rid of ASAP
unsigned int JPGSteg::static_capacity(char *cover_payload, int len)
{
  ssize_t body_offset = extract_appropriate_respones_body(cover_payload, len);
  if (body_offset == -1) //couldn't find the end of header
    return 0;  //useless payload
 
  size_t header_length = body_offset - (size_t)cover_payload;
  
  int from = starting_point((uint8_t*)body_offset, (size_t)len - header_length);
  return min(len - header_length - from - 2 - sizeof(int), (size_t)0); // 2 for FFD9, 4 for len
}


int JPGSteg::encode(uint8_t* data, size_t data_len, uint8_t* cover_payload, size_t cover_len)
{
	int from = starting_point(cover_payload, cover_len);
	memcpy(cover_payload+from, &data_len, sizeof(data_len));
	memcpy(cover_payload+from+sizeof(data_len), data, data_len);
	return 0;
    
}

ssize_t JPGSteg::decode(const uint8_t* cover_payload, size_t cover_len, uint8_t* data)
{
	// TODO: There may be FFDA in the data
    ssize_t from = starting_point(cover_payload, cover_len);
    assert(from >= 0);
    size_t s = (size_t)*(cover_payload+from);

    //We assume the enough mem is allocated for the data
    assert((size_t)s < c_HTTP_MSG_BUF_SIZE);
	memcpy(data, cover_payload+from+sizeof(int), s);
	return s;

}

/**
   constructor just to call parent constructor
*/
JPGSteg::JPGSteg(PayloadServer* payload_provider, double noise2signal)
  :FileStegMod(payload_provider, noise2signal, HTTP_CONTENT_JPEG)
{
}
