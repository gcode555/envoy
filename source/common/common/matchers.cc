#include "common/common/matchers.h"

#include "envoy/config/core/v3alpha/base.pb.h"
#include "envoy/type/matcher/v3alpha/metadata.pb.h"
#include "envoy/type/matcher/v3alpha/number.pb.h"
#include "envoy/type/matcher/v3alpha/string.pb.h"
#include "envoy/type/matcher/v3alpha/value.pb.h"

#include "common/common/regex.h"
#include "common/config/metadata.h"

#include "absl/strings/match.h"

namespace Envoy {
namespace Matchers {

ValueMatcherConstSharedPtr
ValueMatcher::create(const envoy::type::matcher::v3alpha::ValueMatcher& v) {
  switch (v.match_pattern_case()) {
  case envoy::type::matcher::v3alpha::ValueMatcher::MatchPatternCase::kNullMatch:
    return std::make_shared<const NullMatcher>();
  case envoy::type::matcher::v3alpha::ValueMatcher::MatchPatternCase::kDoubleMatch:
    return std::make_shared<const DoubleMatcher>(v.double_match());
  case envoy::type::matcher::v3alpha::ValueMatcher::MatchPatternCase::kStringMatch:
    return std::make_shared<const StringMatcherImpl>(v.string_match());
  case envoy::type::matcher::v3alpha::ValueMatcher::MatchPatternCase::kBoolMatch:
    return std::make_shared<const BoolMatcher>(v.bool_match());
  case envoy::type::matcher::v3alpha::ValueMatcher::MatchPatternCase::kPresentMatch:
    return std::make_shared<const PresentMatcher>(v.present_match());
  case envoy::type::matcher::v3alpha::ValueMatcher::MatchPatternCase::kListMatch:
    return std::make_shared<const ListMatcher>(v.list_match());
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

bool NullMatcher::match(const ProtobufWkt::Value& value) const {
  return value.kind_case() == ProtobufWkt::Value::kNullValue;
}

bool BoolMatcher::match(const ProtobufWkt::Value& value) const {
  return value.kind_case() == ProtobufWkt::Value::kBoolValue && matcher_ == value.bool_value();
}

bool PresentMatcher::match(const ProtobufWkt::Value& value) const {
  return matcher_ && value.kind_case() != ProtobufWkt::Value::KIND_NOT_SET;
}

bool DoubleMatcher::match(const ProtobufWkt::Value& value) const {
  if (value.kind_case() != ProtobufWkt::Value::kNumberValue) {
    return false;
  }

  const double v = value.number_value();
  switch (matcher_.match_pattern_case()) {
  case envoy::type::matcher::v3alpha::DoubleMatcher::MatchPatternCase::kRange:
    return matcher_.range().start() <= v && v < matcher_.range().end();
  case envoy::type::matcher::v3alpha::DoubleMatcher::MatchPatternCase::kExact:
    return matcher_.exact() == v;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  };
}

StringMatcherImpl::StringMatcherImpl(const envoy::type::matcher::v3alpha::StringMatcher& matcher)
    : matcher_(matcher) {
  if (matcher.match_pattern_case() ==
      envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kHiddenEnvoyDeprecatedRegex) {
    regex_ =
        Regex::Utility::parseStdRegexAsCompiledMatcher(matcher_.hidden_envoy_deprecated_regex());
  } else if (matcher.match_pattern_case() ==
             envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kSafeRegex) {
    regex_ = Regex::Utility::parseRegex(matcher_.safe_regex());
  }
}

bool StringMatcherImpl::match(const ProtobufWkt::Value& value) const {
  if (value.kind_case() != ProtobufWkt::Value::kStringValue) {
    return false;
  }

  return match(value.string_value());
}

bool StringMatcherImpl::match(const absl::string_view value) const {
  switch (matcher_.match_pattern_case()) {
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kExact:
    return matcher_.exact() == value;
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kPrefix:
    return absl::StartsWith(value, matcher_.prefix());
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kSuffix:
    return absl::EndsWith(value, matcher_.suffix());
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kHiddenEnvoyDeprecatedRegex:
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kSafeRegex:
    return regex_->match(value);
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

bool LowerCaseStringMatcher::match(const absl::string_view value) const {
  return matcher_.match(value);
}

bool LowerCaseStringMatcher::match(const ProtobufWkt::Value& value) const {
  return matcher_.match(value);
}

envoy::type::matcher::v3alpha::StringMatcher
LowerCaseStringMatcher::toLowerCase(const envoy::type::matcher::v3alpha::StringMatcher& matcher) {
  envoy::type::matcher::v3alpha::StringMatcher lowercase;
  switch (matcher.match_pattern_case()) {
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kHiddenEnvoyDeprecatedRegex:
    lowercase.set_hidden_envoy_deprecated_regex(
        StringUtil::toLower(matcher.hidden_envoy_deprecated_regex()));
    break;
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kExact:
    lowercase.set_exact(StringUtil::toLower(matcher.exact()));
    break;
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kPrefix:
    lowercase.set_prefix(StringUtil::toLower(matcher.prefix()));
    break;
  case envoy::type::matcher::v3alpha::StringMatcher::MatchPatternCase::kSuffix:
    lowercase.set_suffix(StringUtil::toLower(matcher.suffix()));
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
  return lowercase;
}

ListMatcher::ListMatcher(const envoy::type::matcher::v3alpha::ListMatcher& matcher)
    : matcher_(matcher) {
  ASSERT(matcher_.match_pattern_case() ==
         envoy::type::matcher::v3alpha::ListMatcher::MatchPatternCase::kOneOf);

  oneof_value_matcher_ = ValueMatcher::create(matcher_.one_of());
}

bool ListMatcher::match(const ProtobufWkt::Value& value) const {
  if (value.kind_case() != ProtobufWkt::Value::kListValue) {
    return false;
  }

  if (oneof_value_matcher_) {
    for (const auto& lv : value.list_value().values()) {
      if (oneof_value_matcher_->match(lv)) {
        return true;
      }
    }
  }
  return false;
}

MetadataMatcher::MetadataMatcher(const envoy::type::matcher::v3alpha::MetadataMatcher& matcher)
    : matcher_(matcher) {
  for (const auto& seg : matcher.path()) {
    path_.push_back(seg.key());
  }
  const auto& v = matcher_.value();
  value_matcher_ = ValueMatcher::create(v);
}

bool MetadataMatcher::match(const envoy::config::core::v3alpha::Metadata& metadata) const {
  const auto& value = Envoy::Config::Metadata::metadataValue(metadata, matcher_.filter(), path_);
  return value_matcher_ && value_matcher_->match(value);
}

} // namespace Matchers
} // namespace Envoy
