/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once

#include "frg/spinlock.hpp"
#include "kernel/wait.hpp"
#include "lib/ringbuffer.hpp"
#include <asm-generic/termbits.h>
#include <fs/devfs.hpp>

namespace Gaia::Posix {

class TTY : public Waitable {
public:
  TTY(size_t rows, size_t cols) : rows(rows), cols(cols) {
    termios.c_cc[VEOL] = '\n';
    termios.c_cc[VEOF] = 0;
    termios.c_cc[VERASE] = '\b';
    termios.c_cflag = (CREAD | CS8 | HUPCL);
    termios.c_iflag = (BRKINT | ICRNL | IMAXBEL | IXON | IXANY);
    termios.c_lflag = (ECHO | ICANON | ISIG | IEXTEN | ECHOE);
    termios.c_oflag = (OPOST | ONLCR);
  };

  static void register_tty(TTY *tty, dev_t minor);

  using WriteCallback = void(char c, void *context);

  void input(unsigned char c);

  void set_write_callback(WriteCallback *callback, void *context) {
    this->callback = callback;
    callback_context = context;
  }

  class Ops : public Fs::DeviceOps {
    Result<size_t, Error> write(dev_t minor, frg::span<uint8_t> buf,
                                off_t off) override;

    Result<size_t, Error> read(dev_t minor, frg::span<uint8_t> buf,
                               off_t off) override;

    Result<uint64_t, Error> ioctl(dev_t minor, uint64_t request,
                                  void *arg) override;

    Result<Fs::VnodeAttr, Error> getattr(dev_t minor) override;
  } ops;

  bool in_mediumraw = false;

private:
  // Ringbuffer
  Ringbuffer<unsigned char, 4096> buffer;
  size_t rows, cols;
  frg::simple_spinlock lock;

  struct termios termios;

  Result<Void, Error> push(unsigned char c);
  Result<unsigned char, Error> pop();
  Result<unsigned char, Error> erase();

  WriteCallback *callback;
  void *callback_context;

  inline bool is_cflag_set(int flag) { return termios.c_cflag & flag; }
  inline bool is_iflag_set(int flag) { return termios.c_iflag & flag; }
  inline bool is_oflag_set(int flag) { return termios.c_oflag & flag; }
  inline bool is_lflag_set(int flag) { return termios.c_lflag & flag; }
};

} // namespace Gaia::Posix