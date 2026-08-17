// #include <stdio.h> Cant use them as they operate in user mode, in kernel mode
// we cant use them
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>

static struct usb_device *device;

static struct usb_device_id skel_table[] = {{USB_DEVICE(0x0781, 0x5567)}, {}};
MODULE_DEVICE_TABLE(usb, skel_table);

/* Sysfs attribute callback: exposes real-time USB telemetry to user space */
static ssize_t usb_stats_show(struct device *dev,
                              struct device_attribute *attr, char *buf) {
  struct usb_interface *intf = to_usb_interface(dev);
  struct usb_device *udev = interface_to_usbdev(intf);
  char mfg[64] = "Unknown", prod[64] = "Unknown";

  if (udev->descriptor.iManufacturer)
    usb_string(udev, udev->descriptor.iManufacturer, mfg, sizeof(mfg));
  if (udev->descriptor.iProduct)
    usb_string(udev, udev->descriptor.iProduct, prod, sizeof(prod));

  return scnprintf(buf, PAGE_SIZE,
                   "VID:PID       : %04X:%04X\n"
                   "Manufacturer  : %s\n"
                   "Product       : %s\n"
                   "Endpoints     : %d\n"
                   "Speed         : %s\n",
                   le16_to_cpu(udev->descriptor.idVendor),
                   le16_to_cpu(udev->descriptor.idProduct),
                   mfg, prod,
                   intf->cur_altsetting->desc.bNumEndpoints,
                   usb_speed_string(udev->speed));
}
static DEVICE_ATTR_RO(usb_stats);

static int skel_probe(struct usb_interface *interface,
                      const struct usb_device_id *id) {
  struct usb_host_interface *iface_desc;
  struct usb_endpoint_descriptor *endpoint;
  char str[64];
  int i;

  iface_desc = interface->cur_altsetting;
  printk(KERN_INFO "Pen i/f %d now probed: (%04X:%04X)\n",
         iface_desc->desc.bInterfaceNumber, id->idVendor, id->idProduct);
  printk(KERN_INFO "ID->bNumEndpoints: %02X\n", iface_desc->desc.bNumEndpoints);
  printk(KERN_INFO "ID->bInterfaceClass: %02X\n",
         iface_desc->desc.bInterfaceClass);

  device = interface_to_usbdev(interface);

  /* Query and print human-readable Manufacturer & Product names */
  if (device->descriptor.iManufacturer &&
      usb_string(device, device->descriptor.iManufacturer, str, sizeof(str)) > 0)
    printk(KERN_INFO "Manufacturer : %s\n", str);

  if (device->descriptor.iProduct &&
      usb_string(device, device->descriptor.iProduct, str, sizeof(str)) > 0)
    printk(KERN_INFO "Product Name : %s\n", str);

  for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
    const char *type;
    endpoint = &iface_desc->endpoint[i].desc;
    printk(KERN_INFO "ED[%d]->bEndpointAddress: 0x%02X\n", i,
           endpoint->bEndpointAddress);
    printk(KERN_INFO "ED[%d]->bmAttributes: 0x%02X\n", i,
           endpoint->bmAttributes);
    printk(KERN_INFO "ED[%d]->wMaxPacketSize: 0x%04X (%d)\n", i,
           endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);

    /* Decode endpoint transfer type & direction */
    switch (usb_endpoint_type(endpoint)) {
      case USB_ENDPOINT_XFER_BULK: type = "Bulk"; break;
      case USB_ENDPOINT_XFER_INT:  type = "Interrupt"; break;
      case USB_ENDPOINT_XFER_ISOC: type = "Isochronous"; break;
      default:                     type = "Control"; break;
    }
    printk(KERN_INFO "--> Endpoint %d is %s %s\n", i, type,
           usb_endpoint_dir_in(endpoint) ? "IN" : "OUT");
  }

  /* Register sysfs attribute for user-space telemetry (/sys/.../usb_stats) */
  if (device_create_file(&interface->dev, &dev_attr_usb_stats))
    dev_warn(&interface->dev, "Failed to create sysfs usb_stats file\n");

  return 0;
}

static void skel_disconnect(struct usb_interface *interface) {
  device_remove_file(&interface->dev, &dev_attr_usb_stats);
  usb_put_dev(device);
  printk(KERN_INFO "Pen drive removed\n");
}

static struct usb_driver skel_driver = {
    .name = "usb_driver",
    .probe = skel_probe,
    .disconnect = skel_disconnect,
    .id_table = skel_table,
    .supports_autosuspend = 1,
};

static int __init usb_skel_init(void) {
  int result;
  result = usb_register(&skel_driver);
  if (result < 0) {
    pr_err("usb registeration failed with %s\n", skel_driver.name);
    return -1;
  }
  printk(KERN_INFO "USB initialised\n");
  return 0;
}

module_init(usb_skel_init);

static void __exit usb_skel_exit(void) { usb_deregister(&skel_driver); }

module_exit(usb_skel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lovepreet");
MODULE_DESCRIPTION("USB pendrive registration driver");
