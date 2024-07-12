/* SPDX-License-Identifier: BSD-2-Clause */
#include <dev/acpi/device.hpp>
#include <dev/devkit/registry.hpp>

namespace Gaia::Dev {
static int num = 0;

bool AcpiDevice::match_properties(Properties &props) {

  bool alt = false;
  bool orig = false;

  Value *val = nullptr;

  if ((val = props.get("altHid"))) {
    alt = devid == val->string;
  }

  if ((val = props.get("hid"))) {
    orig = devid == val->string;
  }

  return alt || orig;
}

char i2hex(uint64_t val, uint32_t pos) {
  auto digit = (val >> pos) & 0xf;
  if (digit < 0xa)
    return '0' + digit;
  else
    return 'A' + (digit - 10);
}

static char *eisaid_to_string(uint32_t num) {
  static char out[8];

  num = __builtin_bswap32(num);

  out[0] = (char)(0x40 + ((num >> 26) & 0x1F));
  out[1] = (char)(0x40 + ((num >> 21) & 0x1F));
  out[2] = (char)(0x40 + ((num >> 16) & 0x1F));
  out[3] = i2hex((uint64_t)num, 12);
  out[4] = i2hex((uint64_t)num, 8);
  out[5] = i2hex((uint64_t)num, 4);
  out[6] = i2hex((uint64_t)num, 0);
  out[7] = 0;

  return out;
}

AcpiDevice::AcpiDevice(lai_variable_t id, lai_nsnode_t *obj)
    : aml_node(obj), id(id), driver(Vm::get_allocator()) {
  frg::output_to(_name) << frg::fmt("AcpiDevice{}", num++);
}

Result<lai_nsnode_t *, Error> AcpiDevice::resolve_path(const char *path) {
  auto ret = lai_resolve_path(aml_node, path);

  if (!ret) {
    return Err(Error::NO_SUCH_FILE_OR_DIRECTORY);
  }

  return Ok(ret);
}

Result<uint64_t, Error> AcpiDevice::eval_path_int(const char *path) {
  auto node = TRY(resolve_path(path));
  LAI_CLEANUP_STATE lai_state_t state;
  LAI_CLEANUP_VAR lai_variable_t var = {};
  uint64_t out = 0;

  lai_init_state(&state);

  if (lai_eval(&var, node, &state)) {
    return Err(Error::UNKNOWN);
  }

  if (lai_obj_get_integer(&var, &out)) {
    return Err(Error::TYPE_MISMATCH);
  }

  return Ok(out);
}

void AcpiDevice::start(Service *provider) {
  if (id.type == LAI_TYPE_STRING) {
    devid = lai_exec_string_access(&id);
  } else {
    devid = eisaid_to_string(id.integer);
  }

  auto props = Properties{frg::hash<frg::string_view>{}};

  props["hid"].string = devid;
  props["altHid"].string = devid;
  props["provider"].string = "AcpiDevice";

  auto res = get_catalog().find_driver(this);

  // If no driver was found, don't waste memory by attaching the device
  if (res.is_ok()) {
    driver = res.unwrap();
    attach(provider);
    driver->start(this);
  } else {
    delete this;
  }
}

} // namespace Gaia::Dev
