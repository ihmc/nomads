// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: subject.proto

#include "subject.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// This is a temporary google only hack
#ifdef GOOGLE_PROTOBUF_ENFORCE_UNIQUENESS
#include "third_party/protobuf/version.h"
#endif
// @@protoc_insertion_point(includes)

namespace measure {
}  // namespace measure
namespace protobuf_subject_2eproto {
void InitDefaults() {
}

const ::google::protobuf::EnumDescriptor* file_level_enum_descriptors[1];
const ::google::protobuf::uint32 TableStruct::offsets[1] = {};
static const ::google::protobuf::internal::MigrationSchema* schemas = NULL;
static const ::google::protobuf::Message* const* file_default_instances = NULL;

void protobuf_AssignDescriptors() {
  AddDescriptors();
  AssignDescriptors(
      "subject.proto", schemas, file_default_instances, TableStruct::offsets,
      NULL, file_level_enum_descriptors, NULL);
}

void protobuf_AssignDescriptorsOnce() {
  static ::google::protobuf::internal::once_flag once;
  ::google::protobuf::internal::call_once(once, protobuf_AssignDescriptors);
}

void protobuf_RegisterTypes(const ::std::string&) GOOGLE_PROTOBUF_ATTRIBUTE_COLD;
void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
}

void AddDescriptorsImpl() {
  InitDefaults();
  static const char descriptor[] GOOGLE_PROTOBUF_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
      "\n\rsubject.proto\022\007measure*\214\004\n\007Subject\022\010\n\004"
      "host\020\000\022\013\n\007network\020\001\022\013\n\007traffic\020\002\022\007\n\003cpu\020"
      "\003\022\n\n\006memory\020\004\022\006\n\002os\020\005\022\022\n\016network_health\020"
      "\006\022\016\n\ndisservice\020\007\022\013\n\007mockets\020\010\022\024\n\020link_d"
      "escription\020\t\022\025\n\021netviewer_request\020\n\022\017\n\013g"
      "eolocation\020\013\022\013\n\007battery\020\014\022\t\n\005group\020\r\022\024\n\020"
      "group_connection\020\016\022\020\n\014group_member\020\017\022\013\n\007"
      "process\020\020\022\021\n\rmember_sensor\020\021\022\021\n\rtopology"
      "_node\020\022\022\021\n\rtopology_edge\020\023\022\022\n\016federation"
      "_log\020\024\022\024\n\020netproxy_process\020\025\022\031\n\025netproxy"
      "_addr_mapping\020\026\022\032\n\026netproxy_proto_mappin"
      "g\020\027\022\026\n\022netproxy_link_desc\020\030\022\031\n\025netproxy_"
      "link_traffic\020\031\022\025\n\021network_interface\020\032\022\007\n"
      "\003rtt\020\033\022\017\n\013packet_loss\020\034\022\006\n\002iw\020\035B:\n\034us.ih"
      "mc.sensei.proto.measureB\014SubjectProtoP\001Z"
      "\007measure\240\001\001b\006proto3"
  };
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
      descriptor, 619);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "subject.proto", &protobuf_RegisterTypes);
}

void AddDescriptors() {
  static ::google::protobuf::internal::once_flag once;
  ::google::protobuf::internal::call_once(once, AddDescriptorsImpl);
}
// Force AddDescriptors() to be called at dynamic initialization time.
struct StaticDescriptorInitializer {
  StaticDescriptorInitializer() {
    AddDescriptors();
  }
} static_descriptor_initializer;
}  // namespace protobuf_subject_2eproto
namespace measure {
const ::google::protobuf::EnumDescriptor* Subject_descriptor() {
  protobuf_subject_2eproto::protobuf_AssignDescriptorsOnce();
  return protobuf_subject_2eproto::file_level_enum_descriptors[0];
}
bool Subject_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
      return true;
    default:
      return false;
  }
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace measure
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google

// @@protoc_insertion_point(global_scope)
