// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: subject.proto

#ifndef PROTOBUF_INCLUDED_subject_2eproto
#define PROTOBUF_INCLUDED_subject_2eproto

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3006001
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_subject_2eproto 

namespace protobuf_subject_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[1];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_subject_2eproto
namespace measure {
}  // namespace measure
namespace measure {

enum Subject {
  host = 0,
  network = 1,
  traffic = 2,
  cpu = 3,
  memory = 4,
  os = 5,
  network_health = 6,
  disservice = 7,
  mockets = 8,
  link_description = 9,
  netviewer_request = 10,
  geolocation = 11,
  battery = 12,
  group = 13,
  group_connection = 14,
  group_member = 15,
  process = 16,
  member_sensor = 17,
  topology_node = 18,
  topology_edge = 19,
  federation_log = 20,
  netproxy_process = 21,
  netproxy_addr_mapping = 22,
  netproxy_proto_mapping = 23,
  netproxy_link_desc = 24,
  netproxy_link_traffic = 25,
  network_interface = 26,
  rtt = 27,
  packet_loss = 28,
  iw = 29,
  Subject_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
  Subject_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
};
bool Subject_IsValid(int value);
const Subject Subject_MIN = host;
const Subject Subject_MAX = iw;
const int Subject_ARRAYSIZE = Subject_MAX + 1;

const ::google::protobuf::EnumDescriptor* Subject_descriptor();
inline const ::std::string& Subject_Name(Subject value) {
  return ::google::protobuf::internal::NameOfEnum(
    Subject_descriptor(), value);
}
inline bool Subject_Parse(
    const ::std::string& name, Subject* value) {
  return ::google::protobuf::internal::ParseNamedEnum<Subject>(
    Subject_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace measure

namespace google {
namespace protobuf {

template <> struct is_proto_enum< ::measure::Subject> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::measure::Subject>() {
  return ::measure::Subject_descriptor();
}

}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_subject_2eproto
