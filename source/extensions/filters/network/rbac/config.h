#pragma once

#include "envoy/extensions/filters/network/rbac/v3alpha/rbac.pb.h"
#include "envoy/extensions/filters/network/rbac/v3alpha/rbac.pb.validate.h"

#include "extensions/filters/network/common/factory_base.h"
#include "extensions/filters/network/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace RBACFilter {

/**
 * Config registration for the RBAC network filter. @see NamedNetworkFilterConfigFactory.
 */
class RoleBasedAccessControlNetworkFilterConfigFactory
    : public Common::FactoryBase<envoy::extensions::filters::network::rbac::v3alpha::RBAC> {

public:
  RoleBasedAccessControlNetworkFilterConfigFactory()
      : FactoryBase(NetworkFilterNames::get().Rbac) {}

private:
  Network::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::network::rbac::v3alpha::RBAC& proto_config,
      Server::Configuration::FactoryContext& context) override;
};

} // namespace RBACFilter
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
