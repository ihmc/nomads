// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: netproxyinfo.proto

#ifndef PROTOBUF_INCLUDED_netproxyinfo_2eproto
#define PROTOBUF_INCLUDED_netproxyinfo_2eproto

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
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
#include "topology.pb.h"
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_netproxyinfo_2eproto 

namespace protobuf_netproxyinfo_2eproto {
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
}  // namespace protobuf_netproxyinfo_2eproto
namespace netsensor {
class NetProxyInfo;
class NetProxyInfoDefaultTypeInternal;
extern NetProxyInfoDefaultTypeInternal _NetProxyInfo_default_instance_;
}  // namespace netsensor
namespace google {
namespace protobuf {
template<> ::netsensor::NetProxyInfo* Arena::CreateMaybeMessage<::netsensor::NetProxyInfo>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace netsensor {

// ===================================================================

class NetProxyInfo : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:netsensor.NetProxyInfo) */ {
 public:
  NetProxyInfo();
  virtual ~NetProxyInfo();

  NetProxyInfo(const NetProxyInfo& from);

  inline NetProxyInfo& operator=(const NetProxyInfo& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  NetProxyInfo(NetProxyInfo&& from) noexcept
    : NetProxyInfo() {
    *this = ::std::move(from);
  }

  inline NetProxyInfo& operator=(NetProxyInfo&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const NetProxyInfo& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const NetProxyInfo* internal_default_instance() {
    return reinterpret_cast<const NetProxyInfo*>(
               &_NetProxyInfo_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(NetProxyInfo* other);
  friend void swap(NetProxyInfo& a, NetProxyInfo& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline NetProxyInfo* New() const final {
    return CreateMaybeMessage<NetProxyInfo>(NULL);
  }

  NetProxyInfo* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<NetProxyInfo>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const NetProxyInfo& from);
  void MergeFrom(const NetProxyInfo& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(NetProxyInfo* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // repeated uint32 remoteNetProxyIPs = 3;
  int remotenetproxyips_size() const;
  void clear_remotenetproxyips();
  static const int kRemoteNetProxyIPsFieldNumber = 3;
  ::google::protobuf::uint32 remotenetproxyips(int index) const;
  void set_remotenetproxyips(int index, ::google::protobuf::uint32 value);
  void add_remotenetproxyips(::google::protobuf::uint32 value);
  const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >&
      remotenetproxyips() const;
  ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >*
      mutable_remotenetproxyips();

  // .netsensor.NetworkInfo internal = 1;
  bool has_internal() const;
  void clear_internal();
  static const int kInternalFieldNumber = 1;
  private:
  const ::netsensor::NetworkInfo& _internal_internal() const;
  public:
  const ::netsensor::NetworkInfo& internal() const;
  ::netsensor::NetworkInfo* release_internal();
  ::netsensor::NetworkInfo* mutable_internal();
  void set_allocated_internal(::netsensor::NetworkInfo* internal);

  // .netsensor.NetworkInfo external = 2;
  bool has_external() const;
  void clear_external();
  static const int kExternalFieldNumber = 2;
  private:
  const ::netsensor::NetworkInfo& _internal_external() const;
  public:
  const ::netsensor::NetworkInfo& external() const;
  ::netsensor::NetworkInfo* release_external();
  ::netsensor::NetworkInfo* mutable_external();
  void set_allocated_external(::netsensor::NetworkInfo* external);

  // @@protoc_insertion_point(class_scope:netsensor.NetProxyInfo)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::RepeatedField< ::google::protobuf::uint32 > remotenetproxyips_;
  mutable int _remotenetproxyips_cached_byte_size_;
  ::netsensor::NetworkInfo* internal_;
  ::netsensor::NetworkInfo* external_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_netproxyinfo_2eproto::TableStruct;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// NetProxyInfo

// .netsensor.NetworkInfo internal = 1;
inline bool NetProxyInfo::has_internal() const {
  return this != internal_default_instance() && internal_ != NULL;
}
inline const ::netsensor::NetworkInfo& NetProxyInfo::_internal_internal() const {
  return *internal_;
}
inline const ::netsensor::NetworkInfo& NetProxyInfo::internal() const {
  const ::netsensor::NetworkInfo* p = internal_;
  // @@protoc_insertion_point(field_get:netsensor.NetProxyInfo.internal)
  return p != NULL ? *p : *reinterpret_cast<const ::netsensor::NetworkInfo*>(
      &::netsensor::_NetworkInfo_default_instance_);
}
inline ::netsensor::NetworkInfo* NetProxyInfo::release_internal() {
  // @@protoc_insertion_point(field_release:netsensor.NetProxyInfo.internal)
  
  ::netsensor::NetworkInfo* temp = internal_;
  internal_ = NULL;
  return temp;
}
inline ::netsensor::NetworkInfo* NetProxyInfo::mutable_internal() {
  
  if (internal_ == NULL) {
    auto* p = CreateMaybeMessage<::netsensor::NetworkInfo>(GetArenaNoVirtual());
    internal_ = p;
  }
  // @@protoc_insertion_point(field_mutable:netsensor.NetProxyInfo.internal)
  return internal_;
}
inline void NetProxyInfo::set_allocated_internal(::netsensor::NetworkInfo* internal) {
  ::google::protobuf::Arena* message_arena = GetArenaNoVirtual();
  if (message_arena == NULL) {
    delete reinterpret_cast< ::google::protobuf::MessageLite*>(internal_);
  }
  if (internal) {
    ::google::protobuf::Arena* submessage_arena = NULL;
    if (message_arena != submessage_arena) {
      internal = ::google::protobuf::internal::GetOwnedMessage(
          message_arena, internal, submessage_arena);
    }
    
  } else {
    
  }
  internal_ = internal;
  // @@protoc_insertion_point(field_set_allocated:netsensor.NetProxyInfo.internal)
}

// .netsensor.NetworkInfo external = 2;
inline bool NetProxyInfo::has_external() const {
  return this != internal_default_instance() && external_ != NULL;
}
inline const ::netsensor::NetworkInfo& NetProxyInfo::_internal_external() const {
  return *external_;
}
inline const ::netsensor::NetworkInfo& NetProxyInfo::external() const {
  const ::netsensor::NetworkInfo* p = external_;
  // @@protoc_insertion_point(field_get:netsensor.NetProxyInfo.external)
  return p != NULL ? *p : *reinterpret_cast<const ::netsensor::NetworkInfo*>(
      &::netsensor::_NetworkInfo_default_instance_);
}
inline ::netsensor::NetworkInfo* NetProxyInfo::release_external() {
  // @@protoc_insertion_point(field_release:netsensor.NetProxyInfo.external)
  
  ::netsensor::NetworkInfo* temp = external_;
  external_ = NULL;
  return temp;
}
inline ::netsensor::NetworkInfo* NetProxyInfo::mutable_external() {
  
  if (external_ == NULL) {
    auto* p = CreateMaybeMessage<::netsensor::NetworkInfo>(GetArenaNoVirtual());
    external_ = p;
  }
  // @@protoc_insertion_point(field_mutable:netsensor.NetProxyInfo.external)
  return external_;
}
inline void NetProxyInfo::set_allocated_external(::netsensor::NetworkInfo* external) {
  ::google::protobuf::Arena* message_arena = GetArenaNoVirtual();
  if (message_arena == NULL) {
    delete reinterpret_cast< ::google::protobuf::MessageLite*>(external_);
  }
  if (external) {
    ::google::protobuf::Arena* submessage_arena = NULL;
    if (message_arena != submessage_arena) {
      external = ::google::protobuf::internal::GetOwnedMessage(
          message_arena, external, submessage_arena);
    }
    
  } else {
    
  }
  external_ = external;
  // @@protoc_insertion_point(field_set_allocated:netsensor.NetProxyInfo.external)
}

// repeated uint32 remoteNetProxyIPs = 3;
inline int NetProxyInfo::remotenetproxyips_size() const {
  return remotenetproxyips_.size();
}
inline void NetProxyInfo::clear_remotenetproxyips() {
  remotenetproxyips_.Clear();
}
inline ::google::protobuf::uint32 NetProxyInfo::remotenetproxyips(int index) const {
  // @@protoc_insertion_point(field_get:netsensor.NetProxyInfo.remoteNetProxyIPs)
  return remotenetproxyips_.Get(index);
}
inline void NetProxyInfo::set_remotenetproxyips(int index, ::google::protobuf::uint32 value) {
  remotenetproxyips_.Set(index, value);
  // @@protoc_insertion_point(field_set:netsensor.NetProxyInfo.remoteNetProxyIPs)
}
inline void NetProxyInfo::add_remotenetproxyips(::google::protobuf::uint32 value) {
  remotenetproxyips_.Add(value);
  // @@protoc_insertion_point(field_add:netsensor.NetProxyInfo.remoteNetProxyIPs)
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >&
NetProxyInfo::remotenetproxyips() const {
  // @@protoc_insertion_point(field_list:netsensor.NetProxyInfo.remoteNetProxyIPs)
  return remotenetproxyips_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >*
NetProxyInfo::mutable_remotenetproxyips() {
  // @@protoc_insertion_point(field_mutable_list:netsensor.NetProxyInfo.remoteNetProxyIPs)
  return &remotenetproxyips_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace netsensor

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_netproxyinfo_2eproto
