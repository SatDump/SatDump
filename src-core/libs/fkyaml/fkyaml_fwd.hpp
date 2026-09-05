//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.4.3
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

#ifndef FK_YAML_FKYAML_FWD_HPP
#define FK_YAML_FKYAML_FWD_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// #include <fkYAML/detail/macros/version_macros.hpp>
//  _______   __ __   __  _____   __  __  __
// |   __| |_/  |  \_/  |/  _  \ /  \/  \|  |     fkYAML: A C++ header-only YAML library
// |   __|  _  < \_   _/|  ___  |    _   |  |___  version 0.4.3
// |__|  |_| \__|  |_|  |_|   |_|___||___|______| https://github.com/fktn-k/fkYAML
//
// SPDX-FileCopyrightText: 2023-2026 Kensuke Fukutani <fktn.dev@gmail.com>
// SPDX-License-Identifier: MIT

// Check version definitions if already defined.
#if defined(FK_YAML_MAJOR_VERSION) && defined(FK_YAML_MINOR_VERSION) && defined(FK_YAML_PATCH_VERSION)
#if FK_YAML_MAJOR_VERSION != 0 || FK_YAML_MINOR_VERSION != 4 || FK_YAML_PATCH_VERSION != 3
#warning Already included a different version of the fkYAML library!
#else
// define macros to skip defining macros down below.
#define FK_YAML_VERCHECK_SUCCEEDED
#endif
#endif

#ifndef FK_YAML_VERCHECK_SUCCEEDED

#define FK_YAML_MAJOR_VERSION 0
#define FK_YAML_MINOR_VERSION 4
#define FK_YAML_PATCH_VERSION 3

#define FK_YAML_NAMESPACE_VERSION_CONCAT_IMPL(major, minor, patch) v##major##_##minor##_##patch

#define FK_YAML_NAMESPACE_VERSION_CONCAT(major, minor, patch) FK_YAML_NAMESPACE_VERSION_CONCAT_IMPL(major, minor, patch)

#define FK_YAML_NAMESPACE_VERSION                                                                                      \
    FK_YAML_NAMESPACE_VERSION_CONCAT(FK_YAML_MAJOR_VERSION, FK_YAML_MINOR_VERSION, FK_YAML_PATCH_VERSION)

#define FK_YAML_NAMESPACE_BEGIN                                                                                        \
    namespace fkyaml {                                                                                                 \
    inline namespace FK_YAML_NAMESPACE_VERSION {

#define FK_YAML_NAMESPACE_END                                                                                          \
    } /* inline namespace FK_YAML_NAMESPACE_VERSION */                                                                 \
    } // namespace fkyaml

#define FK_YAML_DETAIL_NAMESPACE_BEGIN                                                                                 \
    FK_YAML_NAMESPACE_BEGIN                                                                                            \
    namespace detail {

#define FK_YAML_DETAIL_NAMESPACE_END                                                                                   \
    } /* namespace detail */                                                                                           \
    FK_YAML_NAMESPACE_END

#endif // !defined(FK_YAML_VERCHECK_SUCCEEDED)


FK_YAML_NAMESPACE_BEGIN

/// @brief An ADL friendly converter between basic_node objects and native data objects.
/// @tparam ValueType A target data type.
/// @sa https://fktn-k.github.io/fkYAML/api/node_value_converter/
template <typename ValueType, typename = void>
class node_value_converter;

/// @brief A class to store value of YAML nodes.
/// @sa https://fktn-k.github.io/fkYAML/api/basic_node/
template <
    template <typename, typename...> class SequenceType = std::vector,
    template <typename, typename, typename...> class MappingType = std::map, typename BooleanType = bool,
    typename IntegerType = std::int64_t, typename FloatNumberType = double, typename StringType = std::string,
    template <typename, typename = void> class ConverterType = node_value_converter>
class basic_node;

/// @brief default YAML node value container.
/// @sa https://fktn-k.github.io/fkYAML/api/basic_node/node/
using node = basic_node<>;

/// @brief A minimal map-like container which preserves insertion order.
/// @tparam Key A type for keys.
/// @tparam Value A type for values.
/// @tparam IgnoredCompare A placeholder for key comparison. This will be ignored.
/// @tparam Allocator A class for allocators.
/// @sa https://fktn-k.github.io/fkYAML/api/ordered_map/
template <typename Key, typename Value, typename IgnoredCompare, typename Allocator>
class ordered_map;

FK_YAML_NAMESPACE_END

#endif /* FK_YAML_FKYAML_FWD_HPP */
