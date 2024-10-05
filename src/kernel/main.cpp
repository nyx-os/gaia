/* SPDX-License-Identifier: BSD-2-Clause */
#include "lib/charon.hpp"
#include <dev/acpi/acpi.hpp>
#include <dev/builtin.hpp>
#include <dev/console/fbconsole.hpp>
#include <dev/devkit/registry.hpp>
#include <fs/tmpfs.hpp>
#include <fs/vfs.hpp>
#include <hal/hal.hpp>
#include <kernel/elf.hpp>
#include <kernel/main.hpp>
#include <kernel/sched.hpp>
#include <kernel/timer.hpp>
#include <lib/list.hpp>
#include <linux/fb.h>
#include <posix/errno.hpp>
#include <posix/exec.hpp>
#include <posix/fd.hpp>
#include <vm/phys.hpp>
#include <vm/vm.hpp>

using namespace Gaia;

class FbDev : public Fs::DeviceOps {

public:
  FbDev(CharonFramebuffer fb) : fb(fb) {}

  Result<size_t, Error> read(dev_t minor, frg::span<uint8_t> buf,
                             off_t off) override {
    (void)minor;
    (void)buf;
    (void)off;
    return Err(Error::INVALID_FILE);
  }

  Result<size_t, Error> write(dev_t minor, frg::span<uint8_t> buf,
                              off_t off) override {
    (void)minor;
    memcpy((uint8_t *)fb.address + off, buf.data(), buf.size());
    return Ok(buf.size());
  }

  Result<uint64_t, Error> ioctl(dev_t minor, uint64_t request,
                                void *arg) override {

    (void)minor;

    if (request == FBIOGET_VSCREENINFO) {
      struct fb_var_screeninfo *info = (struct fb_var_screeninfo *)arg;
      info->xres = fb.width;
      info->yres = fb.height;
      info->xres_virtual = fb.width;
      info->yres_virtual = fb.height;
      info->bits_per_pixel = fb.bpp;
      info->red.msb_right = 1;
      info->green.msb_right = 1;
      info->blue.msb_right = 1;
      info->transp.msb_right = 1;
      info->red.offset = fb.red_mask_shift;
      info->red.length = fb.red_mask_size;
      info->blue.offset = fb.blue_mask_shift;
      info->blue.length = fb.blue_mask_size;
      info->green.offset = fb.green_mask_shift;
      info->green.length = fb.green_mask_size;
      info->height = -1;
      info->width = -1;
    } else if (request == FBIOGET_FSCREENINFO) {
      struct fb_fix_screeninfo *fi = (struct fb_fix_screeninfo *)arg;

      fi->smem_len = fb.pitch * fb.height;
      fi->type = FB_TYPE_PACKED_PIXELS;
      fi->visual = FB_VISUAL_TRUECOLOR;
      fi->line_length = fb.pitch;
      fi->mmio_len = fb.pitch * fb.height;
    }

    return Ok((uint64_t)0);
  }

  Result<Fs::VnodeAttr, Error> getattr(dev_t minor) override {
    (void)minor;
    auto ret = Fs::VnodeAttr{};

    ret.mode = S_IFCHR;

    return Ok(ret);
  }

private:
  CharonFramebuffer fb;
};

static Charon _charon;

Charon &Gaia::charon() { return _charon; }

Result<Void, Error> Gaia::main(Charon charon) {
  _charon = charon;

  static Dev::FbConsole fbconsole(charon);

  Vm::phys_init(charon);
  Vm::init();

  Dev::create_registry();
  Dev::initialize_kernel_drivers();
  Dev::AcpiPc pc(charon);

  pc.dump_tables();

  /*
   * We want to initialize the scheduler before everything else, so we can use
   * cpu_self() and other goodies like events
   */
  TRY(sched_init());

  Hal::init_devices(&pc);

  pc.load_drivers();

  Fs::tmpfs_init(charon);

  Dev::system_console()->create_dev();

  auto fbdev = new FbDev(charon.framebuffer);

  auto maj = Fs::dev_alloc_major(fbdev).unwrap();

  Fs::vfs_find_and("/dev/fb0", MAKEDEV(maj, 0), Fs::vfs_create_file).unwrap();

  log("Gaia v0.0.1 (git version {}), {} pages are available",
      __GAIA_GIT_VERSION__, Vm::phys_usable_pages());

  auto task =
      TRY(sched_new_task(sched_allocate_pid(), sched_kernel_task(), true));

  const char *argv[] = {"/usr/bin/init", nullptr};
  const char *envp[] = {"SHELL=/usr/bin/bash", nullptr};

  log("Launching init program /usr/bin/init");
  TRY(execve(*task, "/usr/bin/init", argv, envp));

  Dev::system_console()->clear();
  Hal::enable_interrupts();

  return Ok({});
}
