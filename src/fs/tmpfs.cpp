#include "frg/hash_map.hpp"
#include "frg/optional.hpp"
#include "frg/string.hpp"
#include "fs/devfs.hpp"
#include "fs/vfs.hpp"
#include "vm/heap.hpp"
#include <dirent.h>
#include <fs/tmpfs.hpp>

namespace Gaia::Fs {

class Tmpfs;

extern Tmpfs tmpfs;

Vnode *TmpNode::make_vnode(dev_t dev) {
  if (!vnode) {

    if (this->type == Vnode::Type::CHR) {
      vnode = new Vnode(type, this, (VnodeOps *)get_devfs());
      vnode->dev = dev;
    } else {
      vnode = new Vnode(type, this, (VnodeOps *)&tmpfs);
    }
  }

  return vnode;
}

TmpNode::TmpNode(TmpNode *cwd, Vnode::Type type, frg::string_view name,
                 VnodeAttr attr, frg::optional<frg::string_view> symlink_path) {
  auto dirent = new TmpDirent();

  dirent->tnode = this;

  memcpy(dirent->name, name.data(), MAX(name.size(), 256));

  dirent->name[MAX(name.size(), 256) - 1] = 0;

  this->attr = attr;
  this->type = type;
  this->parent = cwd;

  switch (type) {
  case Vnode::Type::REG:
    reg.buffer = nullptr;
    break;
  case Vnode::Type::DIR:
    new (&dir.entries)
        frg::hash_map<frg::string_view, TmpDirent *,
                      frg::hash<frg::string_view>, Vm::HeapAllocator>(
            frg::hash<frg::string_view>{});
    break;

  case Vnode::Type::LNK: {
    if (symlink_path.has_value()) {
      // kinda sucks but it'll do for now
      auto node = cwd->vnode->ops->lookup(cwd->vnode, symlink_path.value());
      this->link.to = node.is_ok() ? (TmpNode *)node.unwrap()->data : nullptr;
      this->link.to_name = symlink_path.value();
    }
    break;
  }
  default:
    break;
  }

  if (cwd) {
    cwd->dir.entries.insert(frg::string_view(dirent->name), dirent);
  }
}

class Tmpfs : public VnodeOps {
public:
  Result<size_t, Error> write(Vnode *vn, frg::span<uint8_t> buf,
                              off_t off) override {
    auto tn = (TmpNode *)vn->data;

    auto nbyte = buf.size();
    while (tn->type == Vnode::Type::LNK) {
      tn = tn->link.to;
    }

    if (tn->type != Vnode::Type::REG) {
      return Err(tn->type == Vnode::Type::DIR ? Error::IS_A_DIRECTORY
                                              : Error::INVALID_PARAMETERS);
    }

    // Copy-on-write files
    if (tn->reg.compressed) {
      auto data = tn->reg.buffer;
      tn->reg.buffer = (uint8_t *)Vm::realloc(tn->reg.buffer, tn->attr.size);
      memcpy(tn->reg.buffer, data, tn->attr.size);
      tn->reg.compressed = false;
    }

    if (off + nbyte > tn->attr.size) {
      tn->attr.size = off + nbyte;
    }

    tn->reg.buffer = (uint8_t *)Vm::realloc(tn->reg.buffer, tn->attr.size);

    memcpy((uint8_t *)tn->reg.buffer + off, buf.data(), nbyte);

    return Ok(nbyte);
  }

  Result<size_t, Error> read(Vnode *vn, frg::span<uint8_t> buf,
                             off_t off) override {
    auto tn = (TmpNode *)vn->data;

    auto nbyte = buf.size();

    while (tn->type == Vnode::Type::LNK) {
      tn = tn->link.to;
    }

    if (tn->type != Vnode::Type::REG) {
      return Err(tn->type == Vnode::Type::DIR ? Error::IS_A_DIRECTORY
                                              : Error::INVALID_PARAMETERS);
    }

    if (off + nbyte > tn->attr.size) {
      nbyte = (tn->attr.size <= (size_t)off) ? 0 : tn->attr.size - off;
    }

    memcpy(buf.data(), (uint8_t *)tn->reg.buffer + off, nbyte);

    return Ok(nbyte);
  }

  Result<Vnode *, Error> create(Vnode *cwd, frg::string_view name,
                                VnodeAttr attr, dev_t dev) override {
    if (cwd->type != Vnode::Type::DIR) {
      return Err(Error::NOT_A_DIRECTORY);
    }

    auto n = new TmpNode((TmpNode *)cwd->data,
                         dev != (dev_t)-1 ? Vnode::Type::CHR : Vnode::Type::REG,
                         name, attr);

    return Ok(n->make_vnode(dev));
  }

  Result<Vnode *, Error> link(Vnode *cwd, frg::string_view path,
                              frg::string_view link, VnodeAttr attr) override {

    if (cwd->type != Vnode::Type::DIR) {
      return Err(Error::NOT_A_DIRECTORY);
    }

    auto n =
        new TmpNode((TmpNode *)cwd->data, Vnode::Type::LNK, path, attr, link);

    return Ok(n->make_vnode());
  }

  Result<Vnode *, Error> lookup(Vnode *cwd, frg::string_view name) override {
    auto *node = (TmpNode *)(cwd->data);

    if (node->type != Vnode::Type::DIR) {
      return Err(Error::NOT_A_DIRECTORY);
    }

    auto ent = node->dir.entries.find(name);

    if (ent == node->dir.entries.end()) {
      return Err(Error::NO_SUCH_FILE_OR_DIRECTORY);
    }

    auto tent = ent->get<1>();

    if (tent->tnode->type == Vnode::Type::LNK && !tent->tnode->link.to) {
      auto node = vfs_find(tent->tnode->link.to_name);

      if (!node.is_ok()) {
        return Err(Error::NOT_FOUND);
      }

      tent->tnode->link.to = (TmpNode *)node.unwrap()->data;
    }

    return Ok(tent->tnode->make_vnode());
  }

  Result<Vnode *, Error> mkdir(Vnode *dir, frg::string_view name,
                               VnodeAttr attr) override {

    auto out = TRY(make_new_dir(dir, name, attr));
    auto n = TRY(make_new_dir(out, "."));

    n->data = out->data;

    n = TRY(make_new_dir(out, ".."));

    n->data = dir->data;

    return Ok(out);
  }

  Result<uint64_t, Error> ioctl(Vnode *node, uint64_t request,
                                void *arg) override {

    (void)node;
    (void)request;
    (void)arg;

    return Err(Error::NOT_A_TTY);
  }

  Result<VnodeAttr, Error> getattr(Vnode *vn) override {
    auto tn = (TmpNode *)vn->data;

    if (tn->type == Vnode::Type::LNK) {
      tn = tn->link.to;
    }

    if (tn->type == Vnode::Type::DIR) {
      tn->attr.mode |= S_IFDIR;
    }

    return Ok(tn->attr);
  }

  Result<size_t, Error> readdir(Vnode *vn, void *buf, size_t max_size,
                                off_t offset) override {

    auto tn = (TmpNode *)vn->data;

    if (tn->type != Vnode::Type::DIR)
      return Err(Error::NOT_A_DIRECTORY);

    struct dirent *dent = (struct dirent *)(buf);
    size_t bytes_written = 0;
    off_t i = 0;

    for (auto ent : tn->dir.entries) {
      if (i >= offset) {
        auto tent = ent.get<1>();

        size_t name_len = strlen(tent->name);
        size_t dirent_size = offsetof(struct dirent, d_name) + name_len + 1;

        // Align dirent size to 8 bytes
        dirent_size = (dirent_size + 7) & ~7;

        // If we're writing out of bounds, abort
        if (bytes_written + dirent_size > max_size)
          break;

        if (tent->tnode->type == Vnode::Type::DIR)
          dent->d_type = DT_DIR;
        else if (tent->tnode->type == Vnode::Type::REG)
          dent->d_type = DT_REG;
        else
          dent->d_type = DT_UNKNOWN;

        dent->d_ino = (ino_t)tent->tnode;
        dent->d_off = i + 1;
        dent->d_reclen = dirent_size;

        strncpy(dent->d_name, tent->name, name_len);
        dent->d_name[name_len] = '\0';

        bytes_written += dirent_size;
        dent = (struct dirent *)((uint8_t *)dent + dirent_size);
      }

      ++i;
    }

    return Ok(bytes_written);
  }

  Vm::String get_absolute_path(Vnode *vn) override {
    auto tn = (TmpNode *)vn->data;

    if (vn == root_vnode) {
      return "/";
    }

    auto get_node_name = [](TmpNode *n) -> char * {
      if (n->parent) {
        for (auto entry : n->parent->dir.entries) {
          auto node = entry.get<1>();
          if (node->tnode == n)
            return node->name;
        }
      } else {
        return (char *)"/";
      }
      return nullptr;
    };

    Vm::Vector<frg::string_view> path_components;

    // TODO: handle if parent is in another filesystem
    // Traverse up to the root vnode
    auto node = tn;
    while (node) {
      auto name = get_node_name(node);
      if (name) {
        path_components.emplace_back(name);
      }
      node = node->parent;
    }

    // Construct the absolute path
    Vm::String absolute_path;
    if (path_components.empty()) {
      absolute_path = "/";
    } else {
      for (auto elem : path_components) {
        absolute_path += "/";
        absolute_path += elem;
      }
    }

    return absolute_path;
  }

  void init() {
    auto root_node = new TmpNode(nullptr, Vnode::Type::DIR, "/");
    root_vnode = root_node->make_vnode();

    auto n = make_new_dir(root_vnode, ".").unwrap();

    n->data = root_node;

    n = make_new_dir(root_vnode, "..").unwrap();

    n->data = root_node;
  }

private:
  Result<Vnode *, Error> make_new_dir(Vnode *vn, frg::string_view name,
                                      VnodeAttr attr = DefaultVnodeAttr) {
    if (vn->type != Vnode::Type::DIR) {
      return Err(Error::NOT_A_DIRECTORY);
    }

    auto n = new TmpNode((TmpNode *)vn->data, Vnode::Type::DIR, name, attr);

    return Ok(n->make_vnode());
  }
};

Tmpfs tmpfs;

static inline uint64_t oct2int(const char *str, size_t len) {
  uint64_t value = 0;
  while (*str && len > 0) {
    value = value * 8 + (*str++ - '0');
    len--;
  }
  return value;
}

struct TarHeader {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char type;
  char link_name[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char dev_major[8];
  char dev_minor[8];
  char prefix[155];
};

void tmpfs_init(Charon charon) {
  devfs_init();

  uint8_t *ramdisk = nullptr;

  for (auto module : charon.modules.modules) {
    if (module.name) {
      if (frg::string_view{module.name} == "/ramdisk.tar") {
        ramdisk = (uint8_t *)module.address;
      }
    }
  }

  tmpfs.init();

  TarHeader *current_file = (TarHeader *)ramdisk;

  log("Loading ramdisk...");

  while (frg::string_view{current_file->magic, 5} == "ustar") {
    uint64_t size = oct2int(current_file->size, sizeof(current_file->size));

    switch (current_file->type) {

    // File
    case '0': {
      VnodeAttr attr{};

      attr.time = oct2int(current_file->mtime, sizeof(current_file->mtime) - 1);

      attr.size = oct2int(current_file->size, sizeof(current_file->size));

      attr.mode = oct2int(current_file->mode, sizeof(current_file->mode) - 1) &
                  ~(S_IFMT);

      auto vnode =
          vfs_find_and(current_file->name, -1, vfs_create_file, nullptr, attr)
              .unwrap();

      auto tn = (TmpNode *)vnode->data;

      tn->reg.compressed = true;
      tn->reg.buffer = (uint8_t *)current_file + 512;

      break;
    }

    // Directory
    case '5':
      vfs_find_and(current_file->name, vfs_mkdir).unwrap();
      break;

    case '2':
      vfs_find_and(current_file->name, current_file->link_name, vfs_link)
          .unwrap();
      break;
    }

    current_file =
        (TarHeader *)((uint8_t *)current_file + 512 + ALIGN_UP(size, 512));
  }

  log("done");

  vfs_find_and("/dev", vfs_mkdir).unwrap();
}

} // namespace Gaia::Fs
