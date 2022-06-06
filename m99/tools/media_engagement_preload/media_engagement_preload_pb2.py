# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: media_engagement_preload.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='media_engagement_preload.proto',
  package='chrome_browser_media',
  syntax='proto2',
  serialized_options=b'H\003',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x1emedia_engagement_preload.proto\x12\x14\x63hrome_browser_media\"\x1e\n\rPreloadedData\x12\r\n\x05\x64\x61\x66sa\x18\x01 \x01(\x0c\x42\x02H\x03'
)




_PRELOADEDDATA = _descriptor.Descriptor(
  name='PreloadedData',
  full_name='chrome_browser_media.PreloadedData',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='dafsa', full_name='chrome_browser_media.PreloadedData.dafsa', index=0,
      number=1, type=12, cpp_type=9, label=1,
      has_default_value=False, default_value=b"",
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=56,
  serialized_end=86,
)

DESCRIPTOR.message_types_by_name['PreloadedData'] = _PRELOADEDDATA
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

PreloadedData = _reflection.GeneratedProtocolMessageType('PreloadedData', (_message.Message,), {
  'DESCRIPTOR' : _PRELOADEDDATA,
  '__module__' : 'media_engagement_preload_pb2'
  # @@protoc_insertion_point(class_scope:chrome_browser_media.PreloadedData)
  })
_sym_db.RegisterMessage(PreloadedData)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)
