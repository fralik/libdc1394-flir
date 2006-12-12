/* $Id$
 *
 * topology.c - Linux IEEE-1394 topology map fetching routine.
 * This routine serves as a temporary replacement for the
 * raw1394GetTopologyMap routine found in version 0.2 of libraw1394.
 * Written 8.12.1999 by Andreas Micklei
 * adapted for integration within Coriander by Damien Douxchamps
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __TOPOLOGY_H__
#define __TOPOLOGY_H__

#include "linux/linux.h"
#include "internal.h"

#define SHIFT_START	30
#define WIDTH_START	2
#define SHIFT_PHY_ID	24
#define WIDTH_PHY_ID	6
#define SHIFT_CONT	23
#define WIDTH_CONT	1
#define SHIFT_L		22
#define WIDTH_L		1
#define SHIFT_GAP_CNT	16
#define WIDTH_GAP_CNT	6
#define SHIFT_SP	14
#define WIDTH_SP	2
#define SHIFT_DEL	12
#define WIDTH_DEL	2
#define SHIFT_C		11
#define WIDTH_C		1
#define SHIFT_PWR	8
#define WIDTH_PWR	3
#define SHIFT_P0	6
#define WIDTH_P0	2
#define SHIFT_P1	4
#define WIDTH_P1	2
#define SHIFT_P2	2
#define WIDTH_P2	2
#define SHIFT_I		1
#define WIDTH_I		1
#define SHIFT_M		0
#define WIDTH_M		1

#define SHIFT_N		20
#define WIDTH_N		3
#define SHIFT_RSV	18
#define WIDTH_RSV	2
#define SHIFT_PA	16
#define WIDTH_PA	2
#define SHIFT_PB	14
#define WIDTH_PB	2
#define SHIFT_PC	12
#define WIDTH_PC	2
#define SHIFT_PD	10
#define WIDTH_PD	2
#define SHIFT_PE	8
#define WIDTH_PE	2
#define SHIFT_PF	6
#define WIDTH_PF	2
#define SHIFT_PG	4
#define WIDTH_PG	2
#define SHIFT_PH	2
#define WIDTH_PH	2
#define SHIFT_R		1
#define WIDTH_R		1

int decode_selfid(SelfIdPacket_t *selfid, unsigned int *selfid_raw);

unsigned int bit_extract(unsigned int shift, unsigned int width, unsigned int i);

void decode_selfid_zero(SelfIdPacket_t *p, unsigned int i);

void decode_selfid_more(SelfIdPacket_t *p, unsigned int i);

// the topo map returns an allocated variable. Don't forget to free it!
RAW1394topologyMap *raw1394GetTopologyMap(raw1394handle_t handle);

int cooked1394_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                 size_t length, quadlet_t *buffer);

int cooked1394_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr,
                  size_t length, quadlet_t *data);

#endif // __TOPOLOGY_H__

