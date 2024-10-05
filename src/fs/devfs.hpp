/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/array.hpp>
#include <fs/vfs.hpp>
#include <lib/list.hpp>

#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)

#define MINOR(dev) ((unsigned int)((dev) >> MINORBITS))
#define MAJOR(dev) ((unsigned int)((dev)&MINORMASK))
#define MAKEDEV(ma, mi) (((mi) << MINORBITS) | (ma))

// Pretty much the same thing as tmpfs, but for devices
namespace Gaia::Fs {

class Devfs : public VnodeOps {
public:
  Result<size_t, Error> write(Vnode *vn, frg::span<uint8_t> buf,
                              off_t off) override;

  Result<size_t, Error> read(Vnode *vn, frg::span<uint8_t> buf,
                             off_t off) override;

  Result<Vnode *, Error> create(Vnode *cwd, frg::string_view name,
                                VnodeAttr attr, dev_t dev) override;

  Result<Vnode *, Error> link(Vnode *cwd, frg::string_view path,
                              frg::string_view link, VnodeAttr attr) override;

  Result<Vnode *, Error> lookup(Vnode *cwd, frg::string_view name) override;

  Result<Vnode *, Error> mkdir(Vnode *dir, frg::string_view name,
                               VnodeAttr attr) override;

  Result<uint64_t, Error> ioctl(Vnode *node, uint64_t request,
                                void *arg) override;

  Result<VnodeAttr, Error> getattr(Vnode *node) override;

  Vm::String get_absolute_path(Vnode *node) override;

  Result<size_t, Error> readdir(Vnode *vn, void *buf, size_t max_size,
                                off_t offset) override;
};

class DeviceOps {
public:
  virtual Result<size_t, Error> write(dev_t minor, frg::span<uint8_t> buf,
                                      off_t off) = 0;

  virtual Result<size_t, Error> read(dev_t minor, frg::span<uint8_t> buf,
                                     off_t off) = 0;

  virtual Result<uint64_t, Error> ioctl(dev_t minor, uint64_t request,
                                        void *arg) = 0;

  virtual Result<VnodeAttr, Error> getattr(dev_t minor) = 0;
  // TODO: mmap
};

Result<dev_t, Error> dev_alloc_major(DeviceOps *ops);

void devfs_init();

Devfs *get_devfs();

} // namespace Gaia::Fs
