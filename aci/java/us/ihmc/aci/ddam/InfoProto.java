// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: info.proto

package us.ihmc.aci.ddam;

public final class InfoProto {
  private InfoProto() {}
  public static void registerAllExtensions(
      com.google.protobuf.ExtensionRegistry registry) {
  }
  static com.google.protobuf.Descriptors.Descriptor
    internal_static_ddam_CPU_descriptor;
  static
    com.google.protobuf.GeneratedMessage.FieldAccessorTable
      internal_static_ddam_CPU_fieldAccessorTable;
  static com.google.protobuf.Descriptors.Descriptor
    internal_static_ddam_Network_descriptor;
  static
    com.google.protobuf.GeneratedMessage.FieldAccessorTable
      internal_static_ddam_Network_fieldAccessorTable;
  static com.google.protobuf.Descriptors.Descriptor
    internal_static_ddam_OperatingSystem_descriptor;
  static
    com.google.protobuf.GeneratedMessage.FieldAccessorTable
      internal_static_ddam_OperatingSystem_fieldAccessorTable;
  static com.google.protobuf.Descriptors.Descriptor
    internal_static_ddam_Info_descriptor;
  static
    com.google.protobuf.GeneratedMessage.FieldAccessorTable
      internal_static_ddam_Info_fieldAccessorTable;

  public static com.google.protobuf.Descriptors.FileDescriptor
      getDescriptor() {
    return descriptor;
  }
  private static com.google.protobuf.Descriptors.FileDescriptor
      descriptor;
  static {
    java.lang.String[] descriptorData = {
      "\n\ninfo.proto\022\004ddam\032\037google/protobuf/time" +
      "stamp.proto\"F\n\003CPU\022\016\n\006vendor\030\001 \001(\t\022\r\n\005mo" +
      "del\030\002 \001(\t\022\014\n\004freq\030\003 \001(\021\022\022\n\ntotalCores\030\004 " +
      "\001(\021\"\213\001\n\007Network\022\025\n\rinterfaceName\030\001 \001(\t\022\021" +
      "\n\tipAddress\030\002 \001(\t\022\022\n\nmacAddress\030\003 \001(\t\022\017\n" +
      "\007netmask\030\004 \001(\t\022\021\n\tbroadcast\030\005 \001(\t\022\013\n\003mtu" +
      "\030\006 \001(\022\022\021\n\tisPrimary\030\007 \001(\010\"\213\001\n\017OperatingS" +
      "ystem\022\023\n\013description\030\001 \001(\t\022\014\n\004name\030\002 \001(\t" +
      "\022\014\n\004arch\030\003 \001(\t\022\017\n\007machine\030\004 \001(\t\022\017\n\007versi" +
      "on\030\005 \001(\t\022\016\n\006vendor\030\006 \001(\t\022\025\n\rvendorVersio",
      "n\030\007 \001(\t\"\216\001\n\004Info\022!\n\002os\030\001 \001(\0132\025.ddam.Oper" +
      "atingSystem\022\027\n\004cpus\030\002 \003(\0132\t.ddam.CPU\022\033\n\004" +
      "nics\030\003 \003(\0132\r.ddam.Network\022-\n\ttimestamp\030\004" +
      " \001(\0132\032.google.protobuf.TimestampB\"\n\020us.i" +
      "hmc.aci.ddamB\tInfoProtoP\001\240\001\001b\006proto3"
    };
    com.google.protobuf.Descriptors.FileDescriptor.InternalDescriptorAssigner assigner =
        new com.google.protobuf.Descriptors.FileDescriptor.    InternalDescriptorAssigner() {
          public com.google.protobuf.ExtensionRegistry assignDescriptors(
              com.google.protobuf.Descriptors.FileDescriptor root) {
            descriptor = root;
            return null;
          }
        };
    com.google.protobuf.Descriptors.FileDescriptor
      .internalBuildGeneratedFileFrom(descriptorData,
        new com.google.protobuf.Descriptors.FileDescriptor[] {
          com.google.protobuf.TimestampProto.getDescriptor(),
        }, assigner);
    internal_static_ddam_CPU_descriptor =
      getDescriptor().getMessageTypes().get(0);
    internal_static_ddam_CPU_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessage.FieldAccessorTable(
        internal_static_ddam_CPU_descriptor,
        new java.lang.String[] { "Vendor", "Model", "Freq", "TotalCores", });
    internal_static_ddam_Network_descriptor =
      getDescriptor().getMessageTypes().get(1);
    internal_static_ddam_Network_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessage.FieldAccessorTable(
        internal_static_ddam_Network_descriptor,
        new java.lang.String[] { "InterfaceName", "IpAddress", "MacAddress", "Netmask", "Broadcast", "Mtu", "IsPrimary", });
    internal_static_ddam_OperatingSystem_descriptor =
      getDescriptor().getMessageTypes().get(2);
    internal_static_ddam_OperatingSystem_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessage.FieldAccessorTable(
        internal_static_ddam_OperatingSystem_descriptor,
        new java.lang.String[] { "Description", "Name", "Arch", "Machine", "Version", "Vendor", "VendorVersion", });
    internal_static_ddam_Info_descriptor =
      getDescriptor().getMessageTypes().get(3);
    internal_static_ddam_Info_fieldAccessorTable = new
      com.google.protobuf.GeneratedMessage.FieldAccessorTable(
        internal_static_ddam_Info_descriptor,
        new java.lang.String[] { "Os", "Cpus", "Nics", "Timestamp", });
    com.google.protobuf.TimestampProto.getDescriptor();
  }

  // @@protoc_insertion_point(outer_class_scope)
}
