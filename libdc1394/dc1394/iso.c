
#include "iso.h"
#include "platform.h"
#include "internal.h"

dc1394error_t
dc1394_iso_set_persist (dc1394camera_t * camera)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    dc1394error_t err;
    if ((err = platform_iso_set_persist (cpriv->pcam)) != DC1394_SUCCESS)
        return err;

    cpriv->iso_persist = 1;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_allocate_channel (dc1394camera_t * camera,
        uint64_t channels_allowed, int * channel)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    dc1394error_t err;

    if (channels_allowed == 0) {
        if (camera->bmode_capable)
            channels_allowed = ~((uint64_t)0);
        else
            channels_allowed = 0xffff;
    }

    if ((err = platform_iso_allocate_channel (cpriv->pcam, channels_allowed,
            channel)) != DC1394_SUCCESS)
        return err;

    cpriv->allocated_channels |= ((uint64_t)1 << *channel);
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_release_channel (dc1394camera_t * camera, int channel)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    dc1394error_t err;
    if ((err = platform_iso_release_channel (cpriv->pcam, channel))
            != DC1394_SUCCESS)
        return err;

    cpriv->allocated_channels &= ~((uint64_t)1 << channel);
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_allocate_bandwidth (dc1394camera_t * camera, int bandwidth_units)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    dc1394error_t err;
    if ((err = platform_iso_allocate_bandwidth (cpriv->pcam, bandwidth_units))
            != DC1394_SUCCESS)
        return err;

    cpriv->allocated_bandwidth += bandwidth_units;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_release_bandwidth (dc1394camera_t * camera, int bandwidth_units)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    dc1394error_t err;
    if ((err = platform_iso_release_bandwidth (cpriv->pcam, bandwidth_units))
            != DC1394_SUCCESS)
        return err;

    cpriv->allocated_bandwidth -= bandwidth_units;
    if (cpriv->allocated_bandwidth < 0)
        cpriv->allocated_bandwidth = 0;
    return DC1394_SUCCESS;
}

dc1394error_t
dc1394_iso_release_all (dc1394camera_t * camera)
{
    dc1394camera_priv_t * cpriv = DC1394_CAMERA_PRIV (camera);
    int i;
    for (i = 0; i < 64; i++)
        if (cpriv->allocated_channels & ((uint64_t)1 << i))
            dc1394_iso_release_channel (camera, i);
    if (cpriv->allocated_bandwidth)
        dc1394_iso_release_bandwidth (camera, cpriv->allocated_bandwidth);

    if (cpriv->allocated_bandwidth || cpriv->allocated_channels)
        return DC1394_FAILURE;

    return DC1394_SUCCESS;
}
