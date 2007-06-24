/*
 * Juju backend for dc1394
 * Copyright (C) 2007 Kristian Hoegsberg <krh@bitplanet.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <poll.h>

#include "juju/juju.h"

#define ptr_to_u64(p) ((__u64)(unsigned long)(p))
#define u64_to_ptr(p) ((void *)(unsigned long)(p))

static dc1394error_t
init_frame(dc1394camera_t *camera, int index, dc1394video_frame_t *proto)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  const int N = 8;	/* Number of iso packets per fw_cdev_iso_packet. */
  struct juju_frame *f = craw->frames + index;
  size_t total, payload_length;
  int i, count;

  memcpy (&f->frame, proto, sizeof f->frame);
  f->frame.image = craw->buffer + index * proto->total_bytes;
  f->frame.id = index;
  count = (proto->packets_per_frame + N - 1) / N;
  f->size = count * sizeof *f->packets;
  f->packets = malloc(f->size);
  if (f->packets == NULL)
    return DC1394_MEMORY_ALLOCATION_FAILURE;

  memset(f->packets, 0, f->size);

  total = proto->packets_per_frame * proto->bytes_per_packet;
  for (i = 0; i < count; i++) {
    payload_length = proto->bytes_per_packet * N;
    if (payload_length < total)
      f->packets[i].control = FW_CDEV_ISO_PAYLOAD_LENGTH(payload_length);
    else
      f->packets[i].control = FW_CDEV_ISO_PAYLOAD_LENGTH(total);
    f->packets[i].control |= FW_CDEV_ISO_HEADER_LENGTH(4 * N);
    total -= payload_length;
  }
  f->packets[0].control |= FW_CDEV_ISO_SKIP;
  f->packets[i - 1].control |= FW_CDEV_ISO_INTERRUPT;

  return DC1394_SUCCESS;
}

static void
release_frame(dc1394camera_t *camera, int index)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  struct juju_frame *f = craw->frames + index;
  
  free(f->packets);
}

dc1394error_t
queue_frame (dc1394camera_t *camera, int index)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  struct juju_frame *f = craw->frames + index;
  struct fw_cdev_queue_iso queue;
  int retval;

  queue.size = f->size;
  queue.data = ptr_to_u64(f->frame.image);
  queue.packets = ptr_to_u64(f->packets);
  queue.handle = craw->iso_handle;

  retval = ioctl(craw->iso_fd, FW_CDEV_IOC_QUEUE_ISO, &queue);
  if (retval < 0) {
    fprintf(stderr, "queue_iso failed; %m\n");
    return DC1394_IOCTL_FAILURE;
  }

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_capture_setup(dc1394camera_t *camera, uint32_t num_dma_buffers,
		     uint32_t flags)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  struct fw_cdev_create_iso_context create;
  struct fw_cdev_start_iso start_iso;
  dc1394error_t err;
  dc1394video_frame_t proto;
  int i, j, retval;

  if (flags & DC1394_CAPTURE_FLAGS_DEFAULT)
    flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC |
        DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC;

  craw->flags = flags;

  // allocate channel/bandwidth if requested
  if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC) {
    err = dc1394_allocate_iso_channel(camera);
    if (err != DC1394_SUCCESS)
      return err;
  }
  if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC) {
    err = dc1394_allocate_bandwidth(camera);
    if (err != DC1394_SUCCESS)
      goto error_channel;
    DC1394_ERR_RTN(err,"Could not allocate bandwidth!");
  }

  err = DC1394_FAILURE;
  craw->iso_fd = open(craw->filename, O_RDWR);
  if (craw->iso_fd < 0)
    goto error_bandwidth;

  create.type = FW_CDEV_ISO_CONTEXT_RECEIVE;
  create.header_size = 4;
  create.channel = camera->iso_channel;
  create.speed = SCODE_400;
  err = DC1394_IOCTL_FAILURE;
  if (ioctl(craw->iso_fd, FW_CDEV_IOC_CREATE_ISO_CONTEXT, &create) < 0)
    goto error_fd;

  craw->iso_handle = create.handle;

  err = _dc1394_capture_basic_setup(camera, &proto);
  if (err != DC1394_SUCCESS)
    goto error_fd;

  craw->num_frames = num_dma_buffers;
  craw->current = -1;
  craw->ready_frames = 0;
  craw->buffer_size = proto.total_bytes * num_dma_buffers;
  craw->buffer =
    mmap(NULL, craw->buffer_size, PROT_READ, MAP_SHARED, craw->iso_fd, 0);
  err = DC1394_IOCTL_FAILURE;
  if (craw->buffer == MAP_FAILED)
    goto error_fd;

  err = DC1394_MEMORY_ALLOCATION_FAILURE;
  craw->frames = malloc (num_dma_buffers * sizeof *craw->frames);
  if (craw->frames == NULL)
    goto error_mmap;

  for (i = 0; i < num_dma_buffers; i++) {
    err = init_frame(camera, i, &proto);
    if (err != DC1394_SUCCESS)
      break;
  }
  if (err != DC1394_SUCCESS) {
    for (j = 0; j < i; j++)
      release_frame(camera, j);
    goto error_mmap;
  }

  for (i = 0; i < num_dma_buffers; i++) {
    err = queue_frame(camera, i);
    if (err != DC1394_SUCCESS)
      goto error_frames;
  }
	
  // starting from here we use the ISO channel so we set the flag in
  // the camera struct:
  camera->capture_is_set = 1;

  start_iso.cycle   = -1;
  start_iso.tags = FW_CDEV_ISO_CONTEXT_MATCH_ALL_TAGS;
  start_iso.sync = 1;
  start_iso.handle = craw->iso_handle;
  retval = ioctl(craw->iso_fd, FW_CDEV_IOC_START_ISO, &start_iso);
  err = DC1394_IOCTL_FAILURE;
  if (retval < 0)
    goto error_frames;

  return DC1394_SUCCESS;

 error_frames:
  for (i = 0; i < num_dma_buffers; i++)
    release_frame(camera, i);
 error_mmap:
  munmap(craw->buffer, craw->buffer_size);
 error_fd:
  close(craw->iso_fd);
 error_bandwidth:
  if (flags & DC1394_CAPTURE_FLAGS_BANDWIDTH_ALLOC)
    dc1394_free_bandwidth(camera);
 error_channel:
  if (flags & DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC)
    dc1394_free_iso_channel(camera);

  return err;
}

dc1394error_t
dc1394_capture_stop(dc1394camera_t *camera)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);

  if (camera->capture_is_set == 0)
    return DC1394_CAPTURE_IS_NOT_SET;

  if (ioctl(craw->iso_fd, FW_CDEV_IOC_STOP_ISO) < 0) 
    return DC1394_IOCTL_FAILURE;

  munmap(craw->buffer, craw->buffer_size);
  close(craw->iso_fd);
  free (craw->frames);
  craw->frames = NULL;
  camera->capture_is_set = 0;

  return DC1394_SUCCESS;
}


dc1394error_t
dc1394_capture_dequeue (dc1394camera_t * camera,
			dc1394capture_policy_t policy,
			dc1394video_frame_t **frame_return)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);
  struct pollfd fds[1];
  struct juju_frame *f;
  int err, timeout, len;
  struct {
    struct fw_cdev_event_iso_interrupt i;
    __u32 headers[256];
  } iso;

  fds[0].fd = craw->iso_fd;
  fds[0].events = POLLIN;

  switch (policy) {
  case DC1394_CAPTURE_POLICY_POLL:
    timeout = 0;
    break;
  case DC1394_CAPTURE_POLICY_WAIT:
  default:
    timeout = -1;
    break;
  }

  while (craw->ready_frames == 0) {
    err = poll(fds, 1, timeout);
    if (err < 0) {
      fprintf(stderr, "poll() failed for device %s\n", craw->filename);
      return DC1394_FAILURE;
    } else if (err == 0) {
      return DC1394_NO_FRAME;
    }

    len = read (craw->iso_fd, &iso, sizeof iso);
    if (len < 0) {
      fprintf (stderr, "failed to read response: %m\n");
      return DC1394_FAILURE;
    }

    if (iso.i.type == FW_CDEV_EVENT_ISO_INTERRUPT)
      craw->ready_frames++;
  }

  craw->current = (craw->current + 1) % craw->num_frames;
  f = craw->frames + craw->current;
  craw->ready_frames--;

  f->frame.frames_behind = craw->ready_frames;
  f->frame.timestamp = 0;

  *frame_return = &f->frame;

  return DC1394_SUCCESS;
}

dc1394error_t
dc1394_capture_enqueue (dc1394camera_t * camera,
			dc1394video_frame_t * frame)
{
  int err; 

  err = DC1394_INVALID_ARGUMENT_VALUE;
  if (frame->camera != camera)
    DC1394_ERR_RTN(err, 
		   "(%s) dc1394_capture_enqueue_dma: "
		   "camera does not match frame's camera\n", __FILE__);

  err = queue_frame (camera, frame->id);
  DC1394_ERR_RTN(err, "Failed to queue frame");

  return DC1394_SUCCESS;
}

int
dc1394_capture_get_fileno (dc1394camera_t * camera)
{
  DC1394_CAST_CAMERA_TO_JUJU(craw, camera);

  return craw->iso_fd;
}
