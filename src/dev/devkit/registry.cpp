#include "frg/logging.hpp"
#include "vm/heap.hpp"
#include <dev/devkit/registry.hpp>
#include <dev/virtio/block.hpp>
#include <frg/manual_box.hpp>
#include <lib/dot.hpp>
#include <utility>

namespace Gaia::Dev {
static frg::manual_box<Registry> registry{};
static frg::manual_box<Catalog> catalog{};

Registry &get_registry() { return (*registry.get()); }

Catalog &get_catalog() { return *catalog.get(); }

void create_registry() {
  registry.initialize();
  catalog.initialize();
}

void Catalog::register_driver(Driver driver, Properties &props) {
  spinlock.lock();
  drivers.push_back(CatalogEntry{props, driver});
  spinlock.unlock();
}

Result<Vm::UniquePtr<Service>, Error> Catalog::find_driver(Service *provider) {
  spinlock.lock();

  for (auto driver : drivers) {

    // Eliminate drivers that don't need the provider
    if (driver.props["provider"].string != provider->class_name())
      continue;

    // Do a personality-based match
    if (!provider->match_properties(driver.props))
      continue;

    // Else, Resort to probing

    // Create the class instance
    if (!driver.driver.init) {
      continue;
    }

    auto service = driver.driver.init();

    // Probe the service
    auto ret = service->probe(provider);

    if (!ret) {
      service.reset(nullptr);
      break;
    } else {
      spinlock.unlock();
      return Ok(std::move(service));
    }
  }

  spinlock.unlock();
  return Err(Error::NOT_FOUND);
}

void Registry::add(Service *client, Service *provider) {
  spinlock.lock();
  auto prov_node = reinterpret_cast<Node *>(provider->node);
  client->node = new Node;
  prov_node->clients.push_back(client);
  spinlock.unlock();
}

void Registry::set_root(Service *service) {
  spinlock.lock();
  if (!root) {
    root = frg::make_unique<Node>(Vm::get_allocator());
  }

  root->provider = service;
  root->clients.clear();

  // FIXME: may lead to a leak?
  service->node = root.get();

  spinlock.unlock();
}

void Registry::add_to_graph(DotGraph<Vm::HeapAllocator> &graph, Service *parent,
                            Service *child) {
  auto child_node = reinterpret_cast<Node *>(child->node);

  frg::string<Vm::HeapAllocator> child_label = "";

  frg::output_to(child_label)
      << frg::fmt("{} (class {})", child->name(), child->class_name());

  if (parent)
    graph.add_child_to_node(parent->name(), child->name(), child_label);
  else
    graph.add_node(child->name(), child_label);

  for (auto _child : child_node->clients) {
    add_to_graph(graph, child, _child);
  }
}

frg::string<Gaia::Vm::HeapAllocator> Registry::make_graph() {
  DotGraph<Vm::HeapAllocator> dot_graph(false);

  add_to_graph(dot_graph, nullptr, root->provider);

  return dot_graph.generate();
}

} // namespace Gaia::Dev
