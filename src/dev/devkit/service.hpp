/* SPDX-License-Identifier: BSD-2-Clause */
#pragma once
#include <frg/hash.hpp>
#include <frg/hash_map.hpp>
#include <frg/string.hpp>
#include <vm/heap.hpp>

namespace Gaia::Dev {

struct Personality {
  frg::string_view provider_name;
  void *match_data;
};

union Value {
  frg::string_view string;
  int integer;
};

using Properties =
    frg::hash_map<frg::string_view, Value, frg::hash<frg::string_view>,
                  Gaia::Vm::HeapAllocator>;

/// The root class for devices and drivers
class Service {
public:
  /**
   * @brief Attaches the client to a provider in the registry
   *
   * @param provider The provider to attach to
   */
  void attach(Service *provider);

  /**
   * @brief Starts the service and associates it with the provider
   *
   * @param provider The provider to associate with
   */
  virtual void start(Service *provider) { attach(provider); };

  /**
   * @brief Probe a matched service to see if it can be used
   *
   * @param provider The matched service
   * @return A Service instance or NULL if probing is unsuccessful.
   */
  virtual Service *probe(Service *provider) {
    (void)provider;
    return this;
  }

  virtual bool match_properties(Properties &properties) {
    (void)properties;
    return true;
  };

  virtual const char *class_name() { return "Service"; }
  virtual const char *name() { return "Service"; }

  virtual ~Service() {}

protected:
  void *node;

  friend class Registry;
};

} // namespace Gaia::Dev
