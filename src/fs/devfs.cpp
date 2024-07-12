#include <fs/devfs.hpp>

namespace Gaia::Fs {

DevOps *devs[64];
size_t current_major = 0;

static Devfs devfs;

Result<dev_t, Error> dev_alloc_major(DevOps *ops) {

  auto ret = current_major++;

  if (ret >= 64) {
    return Err(Error::OUT_OF_MEMORY);
  }

  devs[ret] = ops;

  return Ok(ret);
}

Result<size_t, Error> Devfs::write(Vnode *vn, frg::span<uint8_t> buf,
                                   off_t off) {

  auto major = MAJOR(vn->dev);

  if (major >= current_major) {
    return Err(Error::INVALID_PARAMETERS);
  }

  return devs[major]->write(MINOR(vn->dev), buf, off);
};

Result<size_t, Error> Devfs::read(Vnode *vn, frg::span<uint8_t> buf,
                                  off_t off) {
  auto major = MAJOR(vn->dev);
  if (major >= current_major) {
    return Err(Error::INVALID_PARAMETERS);
  }
  return devs[major]->read(MINOR(vn->dev), buf, off);
};

Result<Vnode *, Error> Devfs::create(Vnode *cwd, frg::string_view name,
                                     VnodeAttr attr, dev_t dev) {
  (void)cwd;
  (void)name;
  (void)attr;
  (void)dev;
  return Err(Error::NOT_IMPLEMENTED);
};

Result<Vnode *, Error> Devfs::link(Vnode *cwd, frg::string_view path,
                                   frg::string_view link, VnodeAttr attr) {
  (void)cwd;
  (void)path;
  (void)link;
  (void)attr;
  return Err(Error::NOT_IMPLEMENTED);
};

Result<Vnode *, Error> Devfs::lookup(Vnode *cwd, frg::string_view name) {
  (void)cwd;
  (void)name;
  ASSERT(false);
  return Err(Error::NOT_IMPLEMENTED);
};

Result<Vnode *, Error> Devfs::mkdir(Vnode *dir, frg::string_view name,
                                    VnodeAttr attr) {
  (void)dir;
  (void)name;
  (void)attr;
  return Err(Error::NOT_IMPLEMENTED);
};

Result<size_t, Error> Devfs::readdir(Vnode *vn, void *buf, size_t max_size,
                                     off_t offset) {
  (void)vn;
  (void)buf;
  (void)max_size;
  (void)offset;
  return Err(Error::NOT_IMPLEMENTED);
};

Result<VnodeAttr, Error> Devfs::getattr(Vnode *vn) {
  auto major = MAJOR(vn->dev);
  if (major >= current_major) {
    return Err(Error::INVALID_PARAMETERS);
  }
  return devs[major]->getattr(MINOR(vn->dev));
};

Result<uint64_t, Error> Devfs::ioctl(Vnode *vn, uint64_t request, void *arg) {
  auto major = MAJOR(vn->dev);
  if (major >= current_major) {
    return Err(Error::INVALID_PARAMETERS);
  }
  return devs[major]->ioctl(MINOR(vn->dev), request, arg);
};

Vm::String Devfs::get_absolute_path(Vnode *node) {
  (void)node;
  return "";
}

void devfs_init() { devfs = Devfs{}; }
Devfs *get_devfs() { return &devfs; }

} // namespace Gaia::Fs
