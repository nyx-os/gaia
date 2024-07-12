#include <lib/log.hpp>

namespace Gaia {
frg::stack_buffer_logger<DebugSink> logger;
frg::simple_spinlock log_lock;
} // namespace Gaia
