#include <dev/console/ps2.hpp>
#include <dev/devkit/registry.hpp>
#include <dev/pci/pcibus.hpp>
#include <dev/virtio/block.hpp>
#include <dev/virtio/virtio.hpp>

namespace Gaia::Dev {
void initialize_kernel_drivers() {
  pcibus_driver_register();
  virtiodevice_driver_register();
  virtioblock_driver_register();
  ps2_driver_register();
}

} // namespace Gaia::Dev
