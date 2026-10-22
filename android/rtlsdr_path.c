/* Keep the bundled implementation and its USB connection ownership unchanged.
 * Select a device by path in the same enumeration that opens it, rather than
 * converting the path to an index for a later, independent enumeration. */
#include "deps/librtlsdr/librtlsdr.c"

int rtlsdr_open_path(rtlsdr_dev_t **out_dev, const char *path)
{
    rtlsdr_dev_t *dev;
    libusb_device **list = NULL;
    ssize_t count;
    int rc;

    if (!out_dev || !path)
        return LIBUSB_ERROR_INVALID_PARAM;
    *out_dev = NULL;
    dev = calloc(1, sizeof(*dev));
    if (!dev)
        return LIBUSB_ERROR_NO_MEM;
    memcpy(dev->fir, fir_default, sizeof(fir_default));
    dev->dev_lost = 1;

    rc = libusb_init(&dev->ctx);
    if (rc < 0)
        goto fail;
    count = libusb_get_device_list(dev->ctx, &list);
    if (count < 0) {
        rc = (int)count;
        goto fail;
    }

    rc = LIBUSB_ERROR_NO_DEVICE;
    for (ssize_t i = 0; i < count; ++i) {
        struct libusb_device_descriptor descriptor;
        char candidate[64];
        if (libusb_get_device_descriptor(list[i], &descriptor) != 0 ||
            !find_known_device(descriptor.idVendor, descriptor.idProduct))
            continue;
        snprintf(candidate, sizeof(candidate), "/dev/bus/usb/%03u/%03u",
                 libusb_get_bus_number(list[i]), libusb_get_device_address(list[i]));
        if (strcmp(candidate, path) != 0)
            continue;
        rc = libusb_open(list[i], &dev->devh);
        break;
    }
    libusb_free_device_list(list, 1);
    list = NULL;
    if (rc != 0)
        goto fail;
    /* rtlsdr_setup owns dev on both success and failure. */
    return rtlsdr_setup(out_dev, dev);

fail:
    if (list)
        libusb_free_device_list(list, 1);
    if (dev->devh)
        libusb_close(dev->devh);
    if (dev->ctx)
        libusb_exit(dev->ctx);
    free(dev);
    return rc;
}
