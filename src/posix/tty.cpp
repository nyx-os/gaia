#include <asm-generic/ioctls.h>
#include <fs/vfs.hpp>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <posix/tty.hpp>
#include <sys/stat.h>

namespace Gaia::Posix {

// ? Not sure about locks here.. rework when implementing SMP.

static TTY *ttys[1];

void TTY::register_tty(TTY *tty, dev_t minor) {
  ASSERT(minor == 0);
  ttys[minor] = tty;
}

Result<Void, Error> TTY::push(unsigned char c) {
  lock.lock();

  TRY(buffer.push(c));

  if (c == termios.c_cc[VEOL] && is_lflag_set(ICANON)) {
    trigger_event(1);
  } else if (!is_lflag_set(ICANON)) {
    trigger_event(1);
  }

  lock.unlock();

  return Ok({});
}

Result<unsigned char, Error> TTY::pop() { return buffer.pop(); }

Result<unsigned char, Error> TTY::erase() {
  if (buffer.size() == 0)
    return Err(Error::EMPTY);

  auto c = TRY(buffer.peek(buffer.size() - 1));
  if (c == termios.c_cc[VEOL])
    return Ok((unsigned char)0);
  TRY(buffer.erase_last());
  return Ok(c);
}

void TTY::input(unsigned char c) {
  auto is_canon = is_lflag_set(ICANON) && !in_mediumraw;

  // Convert NL to CR
  if (c == '\n' && is_iflag_set(INLCR)) {
    c = '\r';
  } else if (c == '\r') {
    // Ignore CR
    if (is_iflag_set(IGNCR))
      return;

    // Convert CR to NL
    if (is_iflag_set(ICRNL)) {
      c = '\n';
    }
  }

  if (is_canon && c == termios.c_cc[VERASE]) {
    if (erase().is_ok()) {
      callback('\b', callback_context);
      callback(' ', callback_context);
      callback('\b', callback_context);
    }

    return;
  }

  if (is_lflag_set(ECHO) || ((c == '\n' && is_lflag_set(ECHONL)))) {
    callback(c, callback_context);
  }

  push(c);
}

Result<size_t, Error> TTY::Ops::write(dev_t minor, frg::span<uint8_t> buf,
                                      off_t off) {

  (void)off;

  auto tty = ttys[minor];

  for (auto c : buf) {
    tty->callback((char)c, tty->callback_context);
  }

  return Ok(buf.size());
}

Result<size_t, Error> TTY::Ops::read(dev_t minor, frg::span<uint8_t> buf,
                                     off_t off) {

  (void)minor;
  (void)off;

  auto nbyte = buf.size();

  auto tty = ttys[minor];

  if (tty->buffer.is_empty() && tty->in_mediumraw) {
    return Ok(0ul);
  }

  while (tty->buffer.size() == 0) {
    tty->await_event(-1);
  }

  if (nbyte > tty->buffer.size()) {
    nbyte = tty->buffer.size();
  }

  size_t i;

  tty->lock.lock();

  for (i = 0; i < nbyte; i++) {
    if (tty->buffer.is_empty()) {
      break;
    }

    unsigned char c = TRY(tty->pop());
    buf.data()[i] = c;
    if (tty->termios.c_lflag & ICANON && c == tty->termios.c_cc[VEOL]) {
      break;
    }
  }

  tty->lock.unlock();

  return Ok(i + 1);
}

Result<Fs::VnodeAttr, Error> TTY::Ops::getattr(dev_t minor) {

  (void)minor;

  auto ret = Fs::VnodeAttr{};

  ret.mode = S_IFCHR;

  return Ok(ret);
};

Result<uint64_t, Error> TTY::Ops::ioctl(dev_t minor, uint64_t request,
                                        void *arg) {

  auto tty = ttys[minor];

  struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel; /* unused */
    unsigned short ws_ypixel; /* unused */
  };

  switch (request) {
  case TIOCGWINSZ: {
    auto winsz = (winsize *)arg;
    winsz->ws_col = tty->cols;
    winsz->ws_row = tty->rows;
    return Ok((uint64_t)0);
  }

  case TCSETSF:
  case TCSETSW:
  case TCSETS: {
    tty->termios = *(struct termios *)arg;
    return Ok((uint64_t)0);
  }

  case TCGETS: {
    *(struct termios *)arg = tty->termios;
    return Ok((uint64_t)0);
  }

  case TIOCSPGRP:
  case TIOCGPGRP:
    return Err(Error::NOT_IMPLEMENTED);

  case KDSKBMODE: {
    if ((uint64_t)arg == K_MEDIUMRAW) {
      tty->in_mediumraw = true;
    }
    return Ok(0ul);
  }
  default:
    break;
  }

  error("TTY: unimplemented ioctl {}", request);
  return Err(Error::NOT_IMPLEMENTED);
}

} // namespace Gaia::Posix