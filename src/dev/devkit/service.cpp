#include "dev/devkit/registry.hpp"
#include <dev/devkit/service.hpp>

namespace Gaia::Dev {
void Service::attach(Service *provider) { get_registry().add(this, provider); }

} // namespace Gaia::Dev