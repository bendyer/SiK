// -*- Mode: C; c-basic-offset: 8; -*-
//
// Copyright (c) 2012 Andrew Tridgell, All Rights Reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  o Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  o Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//

///
/// @file	status.c
///
/// status reporting code
///

#include <stdarg.h>
#include "radio.h"
#include "packet.h"
#include "timer.h"

extern __xdata uint8_t pbuf[MAX_PACKET_LENGTH];

#define MSG_ID_STATUS 0x11

struct status_report {
	uint8_t packet_hdr;
	uint8_t packet_len;
	uint8_t packet_checksum;
	uint8_t rssi;
	uint8_t remrssi;
	uint8_t txbuf;
	uint8_t noise;
	uint8_t remnoise;
	uint16_t rxerrors;
	uint16_t fixed;
};

/// send a status report packet
void status_report_write(void)
{
	struct status_report *m = (struct status_report *)&pbuf[2];
	uint8_t i, run = 1, block_start = 1;

	m->packet_hdr = MSG_ID_STATUS;
	m->packet_len = sizeof(struct status_report);
	m->packet_checksum = 0x00; // no checksum for this packet, what could
	                           // possibly go wrong?
	m->rxerrors = errors.rx_errors;
	m->fixed    = errors.corrected_packets;
	m->txbuf    = serial_read_space();
	m->rssi     = statistics.average_rssi;
	m->remrssi  = remote_statistics.average_rssi;
	m->noise    = statistics.average_noise;
	m->remnoise = remote_statistics.average_noise;

	// Do an in-place COBS encode on the data, to avoid sending NUL bytes
	// except as packet delimiters
	pbuf[0] = 0x00;
	pbuf[1] = 0x00; // For initial COBS length code
	for (i = 2; i <= sizeof(struct status_report) + 1; i++) {
		if (pbuf[i] == 0x00) {
			pbuf[block_start] = run;
			block_start = i;
			run = 1;
		} else {
			run++;
		}
	}
	pbuf[block_start] = run;
	pbuf[i] = 0x00;

	if (serial_write_space() >= sizeof(struct status_report) + 3) {
		serial_write_buf(pbuf, sizeof(struct status_report) + 3);
	}
}
