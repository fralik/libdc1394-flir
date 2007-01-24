/*
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

#include "topology.h"

#define MAXTRIES 20

void decode_selfid_zero(SelfIdPacket_t *p, unsigned int i)
{
   p->packetZero.designator = bit_extract(SHIFT_START, WIDTH_START, i);
   p->packetZero.phyID = bit_extract(SHIFT_PHY_ID, WIDTH_PHY_ID, i);
   p->packetZero.ZeroOrMore = bit_extract(SHIFT_CONT, WIDTH_CONT, i);
   p->packetZero.linkActive = bit_extract(SHIFT_L, WIDTH_L, i);
   p->packetZero.gapCount = bit_extract(SHIFT_GAP_CNT, WIDTH_GAP_CNT, i);
   p->packetZero.phySpeed = bit_extract(SHIFT_SP, WIDTH_SP, i);
   p->packetZero.phyDelay = bit_extract(SHIFT_DEL, WIDTH_DEL, i);
   p->packetZero.contender = bit_extract(SHIFT_C, WIDTH_C, i);
   p->packetZero.powerClass = bit_extract(SHIFT_PWR, WIDTH_PWR, i);
   p->packetZero.port0 = bit_extract(SHIFT_P0, WIDTH_P0, i);
   p->packetZero.port1 = bit_extract(SHIFT_P1, WIDTH_P1, i);
   p->packetZero.port2 = bit_extract(SHIFT_P2, WIDTH_P2, i);
   p->packetZero.initiatedReset = bit_extract(SHIFT_I, WIDTH_I, i);
   p->packetZero.morePackets = bit_extract(SHIFT_M, WIDTH_M, i);
}

void decode_selfid_more(SelfIdPacket_t *p, unsigned int i)
{
  p->packetMore.designator = bit_extract(SHIFT_START, WIDTH_START, i);
  p->packetMore.phyID = bit_extract(SHIFT_PHY_ID, WIDTH_PHY_ID, i);
  p->packetMore.ZeroOrMore = bit_extract(SHIFT_CONT, WIDTH_CONT, i);
  p->packetMore.packetNumber = bit_extract(SHIFT_N, WIDTH_N, i);
  p->packetMore.rsv = bit_extract(SHIFT_RSV, WIDTH_RSV, i);
  p->packetMore.portA = bit_extract(SHIFT_PA, WIDTH_PA, i);
  p->packetMore.portB = bit_extract(SHIFT_PB, WIDTH_PB, i);
  p->packetMore.portC = bit_extract(SHIFT_PC, WIDTH_PC, i);
  p->packetMore.portD = bit_extract(SHIFT_PD, WIDTH_PD, i);
  p->packetMore.portE = bit_extract(SHIFT_PE, WIDTH_PE, i);
  p->packetMore.portF = bit_extract(SHIFT_PF, WIDTH_PF, i);
  p->packetMore.portG = bit_extract(SHIFT_PG, WIDTH_PG, i);
  p->packetMore.portH = bit_extract(SHIFT_PH, WIDTH_PH, i);
  p->packetMore.r = bit_extract(SHIFT_R, WIDTH_R, i);
  p->packetMore.morePackets = bit_extract(SHIFT_M, WIDTH_M, i);
}

unsigned int bit_extract(unsigned int shift, unsigned int width, unsigned int i)
{
  unsigned int mask = 0;
  while (width>0) {
    mask=mask<<1;
    mask++;
    width--;
  }
  return (i >> shift) & mask;
}

int decode_selfid(SelfIdPacket_t *selfid, unsigned int *selfid_raw)
{
  SelfIdPacket_t *p;
  
  p = selfid;
  decode_selfid_zero(p, selfid_raw[0]);
  if (p->packetZero.morePackets == 0) return 1;
  
  p++;
  decode_selfid_more(p, selfid_raw[1]);
  if (p->packetMore.morePackets == 0) return 2;
  
  p++;
  decode_selfid_more(p, selfid_raw[2]);
  if (p->packetMore.morePackets == 0) return 3;
  
  p++;
  decode_selfid_more(p, selfid_raw[3]);
  
  return 4;
}

RAW1394topologyMap *raw1394GetTopologyMap(raw1394handle_t handle)
{
  RAW1394topologyMap *topoMap;
  int ret,p;
  quadlet_t buf;
  
  memset(&buf, 0, sizeof(quadlet_t)); // init to zero to avoid valgrind errors

  topoMap=(RAW1394topologyMap*)calloc(1,sizeof(RAW1394topologyMap)); // init to zero to avoid valgrind errors

  if ((ret = cooked1394_read(handle, 0xffc0 | raw1394_get_local_id(handle),
			     CSR_REGISTER_BASE + CSR_TOPOLOGY_MAP, 4,
			     (quadlet_t *) &buf)) < 0) {
    free(topoMap);
    return NULL;
  }

  buf = htonl(buf);
  topoMap->length = (u_int16_t) (buf>>16);
  topoMap->crc = (u_int16_t) buf;
  if ((ret = cooked1394_read(handle, 0xffc0 | raw1394_get_local_id(handle),
			     CSR_REGISTER_BASE + CSR_TOPOLOGY_MAP + 4, 4,
			     (quadlet_t *) &topoMap->generationNumber)) < 0) {
    free(topoMap);
    return NULL;
  } 
  
  topoMap->generationNumber = htonl(topoMap->generationNumber);
  
  if ((ret = cooked1394_read(handle, 0xffc0 | raw1394_get_local_id(handle),
			     CSR_REGISTER_BASE + CSR_TOPOLOGY_MAP + 8, 4,
			     (quadlet_t *) &buf)) < 0) {
    free(topoMap);
    return NULL;
  } 

  buf = htonl(buf);
  topoMap->nodeCount = (u_int16_t) (buf>>16);
  topoMap->selfIdCount = (u_int16_t) buf;
  if (cooked1394_read(handle, 0xffc0 | raw1394_get_local_id(handle),
		      CSR_REGISTER_BASE + CSR_TOPOLOGY_MAP + 3*4,
		      (topoMap->length-2)*4, ((quadlet_t *)topoMap)+3) < 0){
    free(topoMap);
    return NULL;
  } 

  for ( p=0 ; p < topoMap->length-2 ; p++) {
    *( ((quadlet_t *)topoMap) +3+p) = 
      htonl( *( ( (quadlet_t *)topoMap ) +3+p )   ); 
  }
  return topoMap;
}

int cooked1394_read(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr, size_t length, quadlet_t *buffer)
{
  int retval, i;
  retval=-1;
  for(i=0; i<MAXTRIES; i++) {
    retval = raw1394_read(handle, node, addr, length, buffer);
    if( retval >= 0 ) return retval;	/* Everything is OK */
    if( errno != EAGAIN ) break;
    usleep(DC1394_SLOW_DOWN);
  }
  //MainError("in raw reading from IEEE1394: ");
  return retval;
}

int cooked1394_write(raw1394handle_t handle, nodeid_t node, nodeaddr_t addr, size_t length, quadlet_t *data)
{
  int retval, i;
  retval=-1;
  for(i=0; i<MAXTRIES; i++) {
    retval = raw1394_write(handle, node, addr, length, data);
    if( retval >= 0 ) return retval;	/* Everything is OK */
    if( errno != EAGAIN ) break;
    usleep(DC1394_SLOW_DOWN);
  }
  //MainError("in raw reading from IEEE1394: ");
  return retval;
}

