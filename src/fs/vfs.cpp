#include <fs/vfs.hpp>
#include <lib/path.hpp>
#include <vm/heap.hpp>

namespace Gaia::Fs {

Vnode *root_vnode;

Result<Vnode *, Error> vfs_create_file(Vnode *cwd, frg::string_view pathname,
                                       VnodeAttr attr, dev_t dev) {
  Path<Gaia::Vm::HeapAllocator> path{pathname};
  auto segments = path.parse();

  ASSERT(cwd);
  ASSERT(cwd->ops);

  return cwd->ops->create(cwd, segments[segments.size() - 1], attr, dev);
}

Result<Vnode *, Error> vfs_link(Vnode *cwd, frg::string_view path,
                                frg::string_view link, VnodeAttr attr) {
  return cwd->ops->link(cwd, path, link, attr);
}

Result<Vnode *, Error> vfs_mkdir(Vnode *cwd, frg::string_view pathname,
                                 VnodeAttr attr) {
  Path<Gaia::Vm::HeapAllocator> path{pathname};
  auto segments = path.parse();

  return cwd->ops->mkdir(cwd, segments[segments.size() - 1], attr);
}

Vm::String vfs_get_absolute_path(Vnode *node) {
  return node->ops->get_absolute_path(node);
}

Result<Vnode *, Error> vfs_find(frg::string_view pathname, Vnode *cwd) {
  return vfs_find_and(
      pathname,
      [](Vnode *vn, frg::string_view name,
         VnodeAttr _) -> Result<Vnode *, Error> {
        (void)_;
        return vn->ops->lookup(vn, name);
      },
      cwd);
}

Result<size_t, Error> vfs_read(Vnode *vn, frg::span<uint8_t> buf, off_t off) {
  return vn->ops->read(vn, buf, off);
}

Result<size_t, Error> vfs_readdir(Vnode *vn, void *buf, size_t max_size,
                                  off_t offset) {
  return vn->ops->readdir(vn, buf, max_size, offset);
}

Result<size_t, Error> vfs_write(Vnode *vn, frg::span<uint8_t> buf, off_t off) {
  return vn->ops->write(vn, buf, off);
}

} // namespace Gaia::Fs
