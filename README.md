# Custom Linux USB Device Driver

A custom Linux Kernel Module (LKM) designed for USB device detection, hardware introspection, descriptor querying, and endpoint parsing.

---

## 📌 Motivation & Need for This Driver

Standard operating systems typically bind generic class drivers (such as `usb-storage` for flash drives or `usbhid` for input devices) to connected USB peripherals. While generic drivers work well for standard use cases, they hide low-level bus mechanics and cannot easily be tailored for custom embedded hardware or firmware diagnostics.

### Why This Custom Driver is Needed:
1. **Low-Level Hardware Introspection**: Directly inspects raw interface descriptors, endpoint attributes, packet sizes, and transfer directions without OS abstraction layers.
2. **Proprietary & Custom USB Hardware Development**: Serves as a foundation for interfacing with custom microcontrollers (STM32, ESP32, Arduino) and FPGA-based USB peripherals where standard drivers do not apply.
3. **Descriptor & Telemetry Extraction**: Queries human-readable vendor and product string descriptors in real time upon connection.
4. **Learning & Kernel Driver Development**: Demonstrates the core mechanics of the Linux USB Core subsystem (`usb_driver`, `usb_device_id`, URB preparation, and probe/disconnect lifecycles).

---

## ✨ Features

- **Automatic Device Probing**: Registers a device table (`MODULE_DEVICE_TABLE`) matching targeted Vendor ID and Product ID (VID:PID).
- **String Descriptor Decoding**: Dynamically queries the USB device's string descriptors via `usb_string()` to extract:
  - **Manufacturer Name** (e.g., `SanDisk`)
  - **Product Name** (e.g., `Cruzer Blade`)
- **Endpoint Analysis & Decoding**:
  - Iterates over all interface endpoints (`bNumEndpoints`).
  - Automatically identifies endpoint transfer types (**Bulk**, **Interrupt**, **Isochronous**, or **Control**).
  - Determines transfer direction (**IN** - Device to Host vs. **OUT** - Host to Device).
  - Reads maximum packet size (`wMaxPacketSize`) and endpoint addresses.
- **Sysfs User-Space Telemetry Interface (`/sys/.../usb_stats`)**:
  - Exposes live device metrics (VID:PID, Manufacturer, Product, Endpoint Count, USB Bus Speed) directly via a read-only sysfs file.
  - Allows user-space applications (scripts, monitoring tools) to read device health without root or parsing `dmesg`.
- **Clean Lifecycle Management**: Gracefully handles device hotplug and removal with proper reference counting (`usb_put_dev()`) and sysfs cleanup.

---

## 📂 Repository Structure

```text
├── README.md               # Project documentation and guide
└── usb_driver/
    ├── Makefile            # Kernel build system configuration (kbuild)
    ├── usb_driver.c        # Main USB kernel module implementation
    ├── usb_transfer.c      # URB transfer logic (extensible)
    └── unbind              # Helper script for unbinding default drivers
```

---

## 🛠️ Prerequisites

To build and run this kernel module, you need a Linux environment with:
- **Linux Kernel Headers** matching your running kernel (`uname -r`)
- **GCC / Build Essentials** (`build-essential`, `kmod`, `make`)

Install dependencies on Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential linux-headers-$(uname -r)
```

---

## 🚀 Building & Usage

### 1. Compile the Module
Navigate to the `usb_driver` directory and run `make`:
```bash
cd usb_driver
make
```
This generates the kernel object file `usb_driver.ko`.

---

### 2. (Optional) Unbind Conflicting Generic Driver
If testing with a standard USB drive (e.g., SanDisk), the kernel may automatically bind the default `usb-storage` driver. To test this custom driver, unbind the default driver:
```bash
# Find your USB bus-port ID using lsusb -t or dmesg
echo "1-5" | sudo tee /sys/bus/usb/drivers/usb-storage/unbind
```

---

### 3. Load the Custom Driver
Insert the kernel module:
```bash
sudo insmod usb_driver.ko
```

Verify that the module is loaded:
```bash
lsmod | grep usb_driver
```

---

### 4. Monitor Kernel Logs
Plug in your USB device and observe the driver probing and decoding the device descriptors:
```bash
sudo dmesg -w
```

**Example Output in `dmesg`:**
```text
[ 1234.567890] USB initialised
[ 1238.102345] Pen i/f 0 now probed: (0781:5567)
[ 1238.102346] ID->bNumEndpoints: 02
[ 1238.102347] ID->bInterfaceClass: 08
[ 1238.103450] Manufacturer : SanDisk
[ 1238.104210] Product Name : Cruzer Blade
[ 1238.104212] ED[0]->bEndpointAddress: 0x81
[ 1238.104213] ED[0]->bmAttributes: 0x02
[ 1238.104214] ED[0]->wMaxPacketSize: 0x0200 (512)
[ 1238.104215] --> Endpoint 0 is Bulk IN
[ 1238.104216] ED[1]->bEndpointAddress: 0x02
[ 1238.104217] ED[1]->bmAttributes: 0x02
[ 1238.104218] ED[1]->wMaxPacketSize: 0x0200 (512)
[ 1238.104219] --> Endpoint 1 is Bulk OUT
```

---

### 5. Query Live Hardware Telemetry via Sysfs
Instead of relying only on `dmesg`, user-space programs can directly query the `/sys` file system:
```bash
cat /sys/bus/usb/drivers/usb_driver/*/usb_stats
```

**Output:**
```text
VID:PID       : 0781:5567
Manufacturer  : SanDisk
Product       : Cruzer Blade
Endpoints     : 2
Speed         : high-speed
```

---

### 6. Unload the Driver
Remove the kernel module when done:
```bash
sudo rmmod usb_driver
```

Clean the build artifacts:
```bash
make clean
```

---

## ⚙️ Customizing Target Hardware (VID:PID)

To target your specific USB device, update the `skel_table` in [`usb_driver/usb_driver.c`](file:///Users/soumalyaghosh/usb_device_driver/usb_driver/usb_driver.c):

```c
static struct usb_device_id skel_table[] = {
    { USB_DEVICE(YOUR_VENDOR_ID, YOUR_PRODUCT_ID) },
    { } /* Terminating entry */
};
```
*(Run `lsusb` to find the VID:PID of any connected USB device).*

---

## 📜 License

This project is licensed under the **GPL-2.0** License.
