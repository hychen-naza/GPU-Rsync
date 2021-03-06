// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: rsync.proto

#ifndef PROTOBUF_INCLUDED_rsync_2eproto
#define PROTOBUF_INCLUDED_rsync_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3007000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3007000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
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
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_rsync_2eproto

// Internal implementation detail -- do not use these members.
struct TableStruct_rsync_2eproto {
  static const ::google::protobuf::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::ParseTable schema[4]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors_rsync_2eproto();
namespace rsync {
class FileChunkInfo;
class FileChunkInfoDefaultTypeInternal;
extern FileChunkInfoDefaultTypeInternal _FileChunkInfo_default_instance_;
class FileHead;
class FileHeadDefaultTypeInternal;
extern FileHeadDefaultTypeInternal _FileHead_default_instance_;
class FileInfo;
class FileInfoDefaultTypeInternal;
extern FileInfoDefaultTypeInternal _FileInfo_default_instance_;
class RsyncReply;
class RsyncReplyDefaultTypeInternal;
extern RsyncReplyDefaultTypeInternal _RsyncReply_default_instance_;
}  // namespace rsync
namespace google {
namespace protobuf {
template<> ::rsync::FileChunkInfo* Arena::CreateMaybeMessage<::rsync::FileChunkInfo>(Arena*);
template<> ::rsync::FileHead* Arena::CreateMaybeMessage<::rsync::FileHead>(Arena*);
template<> ::rsync::FileInfo* Arena::CreateMaybeMessage<::rsync::FileInfo>(Arena*);
template<> ::rsync::RsyncReply* Arena::CreateMaybeMessage<::rsync::RsyncReply>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace rsync {

// ===================================================================

class FileHead final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:rsync.FileHead) */ {
 public:
  FileHead();
  virtual ~FileHead();

  FileHead(const FileHead& from);

  inline FileHead& operator=(const FileHead& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  FileHead(FileHead&& from) noexcept
    : FileHead() {
    *this = ::std::move(from);
  }

  inline FileHead& operator=(FileHead&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor() {
    return default_instance().GetDescriptor();
  }
  static const FileHead& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const FileHead* internal_default_instance() {
    return reinterpret_cast<const FileHead*>(
               &_FileHead_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(FileHead* other);
  friend void swap(FileHead& a, FileHead& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline FileHead* New() const final {
    return CreateMaybeMessage<FileHead>(nullptr);
  }

  FileHead* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<FileHead>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const FileHead& from);
  void MergeFrom(const FileHead& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  #if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  static const char* _InternalParse(const char* begin, const char* end, void* object, ::google::protobuf::internal::ParseContext* ctx);
  ::google::protobuf::internal::ParseFunc _ParseFunc() const final { return _InternalParse; }
  #else
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  #endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FileHead* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string fileName = 1;
  void clear_filename();
  static const int kFileNameFieldNumber = 1;
  const ::std::string& filename() const;
  void set_filename(const ::std::string& value);
  #if LANG_CXX11
  void set_filename(::std::string&& value);
  #endif
  void set_filename(const char* value);
  void set_filename(const char* value, size_t size);
  ::std::string* mutable_filename();
  ::std::string* release_filename();
  void set_allocated_filename(::std::string* filename);

  // int32 fileSize = 2;
  void clear_filesize();
  static const int kFileSizeFieldNumber = 2;
  ::google::protobuf::int32 filesize() const;
  void set_filesize(::google::protobuf::int32 value);

  // int32 chunkSize = 3;
  void clear_chunksize();
  static const int kChunkSizeFieldNumber = 3;
  ::google::protobuf::int32 chunksize() const;
  void set_chunksize(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:rsync.FileHead)
 private:
  class HasBitSetters;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr filename_;
  ::google::protobuf::int32 filesize_;
  ::google::protobuf::int32 chunksize_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_rsync_2eproto;
};
// -------------------------------------------------------------------

class FileInfo final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:rsync.FileInfo) */ {
 public:
  FileInfo();
  virtual ~FileInfo();

  FileInfo(const FileInfo& from);

  inline FileInfo& operator=(const FileInfo& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  FileInfo(FileInfo&& from) noexcept
    : FileInfo() {
    *this = ::std::move(from);
  }

  inline FileInfo& operator=(FileInfo&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor() {
    return default_instance().GetDescriptor();
  }
  static const FileInfo& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const FileInfo* internal_default_instance() {
    return reinterpret_cast<const FileInfo*>(
               &_FileInfo_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  void Swap(FileInfo* other);
  friend void swap(FileInfo& a, FileInfo& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline FileInfo* New() const final {
    return CreateMaybeMessage<FileInfo>(nullptr);
  }

  FileInfo* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<FileInfo>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const FileInfo& from);
  void MergeFrom(const FileInfo& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  #if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  static const char* _InternalParse(const char* begin, const char* end, void* object, ::google::protobuf::internal::ParseContext* ctx);
  ::google::protobuf::internal::ParseFunc _ParseFunc() const final { return _InternalParse; }
  #else
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  #endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FileInfo* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // repeated .rsync.FileChunkInfo chunkInfo = 1;
  int chunkinfo_size() const;
  void clear_chunkinfo();
  static const int kChunkInfoFieldNumber = 1;
  ::rsync::FileChunkInfo* mutable_chunkinfo(int index);
  ::google::protobuf::RepeatedPtrField< ::rsync::FileChunkInfo >*
      mutable_chunkinfo();
  const ::rsync::FileChunkInfo& chunkinfo(int index) const;
  ::rsync::FileChunkInfo* add_chunkinfo();
  const ::google::protobuf::RepeatedPtrField< ::rsync::FileChunkInfo >&
      chunkinfo() const;

  // string fileName = 2;
  void clear_filename();
  static const int kFileNameFieldNumber = 2;
  const ::std::string& filename() const;
  void set_filename(const ::std::string& value);
  #if LANG_CXX11
  void set_filename(::std::string&& value);
  #endif
  void set_filename(const char* value);
  void set_filename(const char* value, size_t size);
  ::std::string* mutable_filename();
  ::std::string* release_filename();
  void set_allocated_filename(::std::string* filename);

  // @@protoc_insertion_point(class_scope:rsync.FileInfo)
 private:
  class HasBitSetters;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::RepeatedPtrField< ::rsync::FileChunkInfo > chunkinfo_;
  ::google::protobuf::internal::ArenaStringPtr filename_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_rsync_2eproto;
};
// -------------------------------------------------------------------

class FileChunkInfo final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:rsync.FileChunkInfo) */ {
 public:
  FileChunkInfo();
  virtual ~FileChunkInfo();

  FileChunkInfo(const FileChunkInfo& from);

  inline FileChunkInfo& operator=(const FileChunkInfo& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  FileChunkInfo(FileChunkInfo&& from) noexcept
    : FileChunkInfo() {
    *this = ::std::move(from);
  }

  inline FileChunkInfo& operator=(FileChunkInfo&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor() {
    return default_instance().GetDescriptor();
  }
  static const FileChunkInfo& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const FileChunkInfo* internal_default_instance() {
    return reinterpret_cast<const FileChunkInfo*>(
               &_FileChunkInfo_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  void Swap(FileChunkInfo* other);
  friend void swap(FileChunkInfo& a, FileChunkInfo& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline FileChunkInfo* New() const final {
    return CreateMaybeMessage<FileChunkInfo>(nullptr);
  }

  FileChunkInfo* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<FileChunkInfo>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const FileChunkInfo& from);
  void MergeFrom(const FileChunkInfo& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  #if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  static const char* _InternalParse(const char* begin, const char* end, void* object, ::google::protobuf::internal::ParseContext* ctx);
  ::google::protobuf::internal::ParseFunc _ParseFunc() const final { return _InternalParse; }
  #else
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  #endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FileChunkInfo* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string checksum2 = 3;
  void clear_checksum2();
  static const int kChecksum2FieldNumber = 3;
  const ::std::string& checksum2() const;
  void set_checksum2(const ::std::string& value);
  #if LANG_CXX11
  void set_checksum2(::std::string&& value);
  #endif
  void set_checksum2(const char* value);
  void set_checksum2(const char* value, size_t size);
  ::std::string* mutable_checksum2();
  ::std::string* release_checksum2();
  void set_allocated_checksum2(::std::string* checksum2);

  // int32 chunkId = 1;
  void clear_chunkid();
  static const int kChunkIdFieldNumber = 1;
  ::google::protobuf::int32 chunkid() const;
  void set_chunkid(::google::protobuf::int32 value);

  // int32 checksum1 = 2;
  void clear_checksum1();
  static const int kChecksum1FieldNumber = 2;
  ::google::protobuf::int32 checksum1() const;
  void set_checksum1(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:rsync.FileChunkInfo)
 private:
  class HasBitSetters;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr checksum2_;
  ::google::protobuf::int32 chunkid_;
  ::google::protobuf::int32 checksum1_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_rsync_2eproto;
};
// -------------------------------------------------------------------

class RsyncReply final :
    public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:rsync.RsyncReply) */ {
 public:
  RsyncReply();
  virtual ~RsyncReply();

  RsyncReply(const RsyncReply& from);

  inline RsyncReply& operator=(const RsyncReply& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  RsyncReply(RsyncReply&& from) noexcept
    : RsyncReply() {
    *this = ::std::move(from);
  }

  inline RsyncReply& operator=(RsyncReply&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor() {
    return default_instance().GetDescriptor();
  }
  static const RsyncReply& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const RsyncReply* internal_default_instance() {
    return reinterpret_cast<const RsyncReply*>(
               &_RsyncReply_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    3;

  void Swap(RsyncReply* other);
  friend void swap(RsyncReply& a, RsyncReply& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline RsyncReply* New() const final {
    return CreateMaybeMessage<RsyncReply>(nullptr);
  }

  RsyncReply* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<RsyncReply>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const RsyncReply& from);
  void MergeFrom(const RsyncReply& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  #if GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  static const char* _InternalParse(const char* begin, const char* end, void* object, ::google::protobuf::internal::ParseContext* ctx);
  ::google::protobuf::internal::ParseFunc _ParseFunc() const final { return _InternalParse; }
  #else
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  #endif  // GOOGLE_PROTOBUF_ENABLE_EXPERIMENTAL_PARSER
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(RsyncReply* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return nullptr;
  }
  inline void* MaybeArenaPtr() const {
    return nullptr;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // bool success = 1;
  void clear_success();
  static const int kSuccessFieldNumber = 1;
  bool success() const;
  void set_success(bool value);

  // @@protoc_insertion_point(class_scope:rsync.RsyncReply)
 private:
  class HasBitSetters;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  bool success_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::TableStruct_rsync_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// FileHead

// string fileName = 1;
inline void FileHead::clear_filename() {
  filename_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& FileHead::filename() const {
  // @@protoc_insertion_point(field_get:rsync.FileHead.fileName)
  return filename_.GetNoArena();
}
inline void FileHead::set_filename(const ::std::string& value) {
  
  filename_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:rsync.FileHead.fileName)
}
#if LANG_CXX11
inline void FileHead::set_filename(::std::string&& value) {
  
  filename_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:rsync.FileHead.fileName)
}
#endif
inline void FileHead::set_filename(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  filename_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:rsync.FileHead.fileName)
}
inline void FileHead::set_filename(const char* value, size_t size) {
  
  filename_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:rsync.FileHead.fileName)
}
inline ::std::string* FileHead::mutable_filename() {
  
  // @@protoc_insertion_point(field_mutable:rsync.FileHead.fileName)
  return filename_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* FileHead::release_filename() {
  // @@protoc_insertion_point(field_release:rsync.FileHead.fileName)
  
  return filename_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void FileHead::set_allocated_filename(::std::string* filename) {
  if (filename != nullptr) {
    
  } else {
    
  }
  filename_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), filename);
  // @@protoc_insertion_point(field_set_allocated:rsync.FileHead.fileName)
}

// int32 fileSize = 2;
inline void FileHead::clear_filesize() {
  filesize_ = 0;
}
inline ::google::protobuf::int32 FileHead::filesize() const {
  // @@protoc_insertion_point(field_get:rsync.FileHead.fileSize)
  return filesize_;
}
inline void FileHead::set_filesize(::google::protobuf::int32 value) {
  
  filesize_ = value;
  // @@protoc_insertion_point(field_set:rsync.FileHead.fileSize)
}

// int32 chunkSize = 3;
inline void FileHead::clear_chunksize() {
  chunksize_ = 0;
}
inline ::google::protobuf::int32 FileHead::chunksize() const {
  // @@protoc_insertion_point(field_get:rsync.FileHead.chunkSize)
  return chunksize_;
}
inline void FileHead::set_chunksize(::google::protobuf::int32 value) {
  
  chunksize_ = value;
  // @@protoc_insertion_point(field_set:rsync.FileHead.chunkSize)
}

// -------------------------------------------------------------------

// FileInfo

// repeated .rsync.FileChunkInfo chunkInfo = 1;
inline int FileInfo::chunkinfo_size() const {
  return chunkinfo_.size();
}
inline void FileInfo::clear_chunkinfo() {
  chunkinfo_.Clear();
}
inline ::rsync::FileChunkInfo* FileInfo::mutable_chunkinfo(int index) {
  // @@protoc_insertion_point(field_mutable:rsync.FileInfo.chunkInfo)
  return chunkinfo_.Mutable(index);
}
inline ::google::protobuf::RepeatedPtrField< ::rsync::FileChunkInfo >*
FileInfo::mutable_chunkinfo() {
  // @@protoc_insertion_point(field_mutable_list:rsync.FileInfo.chunkInfo)
  return &chunkinfo_;
}
inline const ::rsync::FileChunkInfo& FileInfo::chunkinfo(int index) const {
  // @@protoc_insertion_point(field_get:rsync.FileInfo.chunkInfo)
  return chunkinfo_.Get(index);
}
inline ::rsync::FileChunkInfo* FileInfo::add_chunkinfo() {
  // @@protoc_insertion_point(field_add:rsync.FileInfo.chunkInfo)
  return chunkinfo_.Add();
}
inline const ::google::protobuf::RepeatedPtrField< ::rsync::FileChunkInfo >&
FileInfo::chunkinfo() const {
  // @@protoc_insertion_point(field_list:rsync.FileInfo.chunkInfo)
  return chunkinfo_;
}

// string fileName = 2;
inline void FileInfo::clear_filename() {
  filename_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& FileInfo::filename() const {
  // @@protoc_insertion_point(field_get:rsync.FileInfo.fileName)
  return filename_.GetNoArena();
}
inline void FileInfo::set_filename(const ::std::string& value) {
  
  filename_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:rsync.FileInfo.fileName)
}
#if LANG_CXX11
inline void FileInfo::set_filename(::std::string&& value) {
  
  filename_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:rsync.FileInfo.fileName)
}
#endif
inline void FileInfo::set_filename(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  filename_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:rsync.FileInfo.fileName)
}
inline void FileInfo::set_filename(const char* value, size_t size) {
  
  filename_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:rsync.FileInfo.fileName)
}
inline ::std::string* FileInfo::mutable_filename() {
  
  // @@protoc_insertion_point(field_mutable:rsync.FileInfo.fileName)
  return filename_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* FileInfo::release_filename() {
  // @@protoc_insertion_point(field_release:rsync.FileInfo.fileName)
  
  return filename_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void FileInfo::set_allocated_filename(::std::string* filename) {
  if (filename != nullptr) {
    
  } else {
    
  }
  filename_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), filename);
  // @@protoc_insertion_point(field_set_allocated:rsync.FileInfo.fileName)
}

// -------------------------------------------------------------------

// FileChunkInfo

// int32 chunkId = 1;
inline void FileChunkInfo::clear_chunkid() {
  chunkid_ = 0;
}
inline ::google::protobuf::int32 FileChunkInfo::chunkid() const {
  // @@protoc_insertion_point(field_get:rsync.FileChunkInfo.chunkId)
  return chunkid_;
}
inline void FileChunkInfo::set_chunkid(::google::protobuf::int32 value) {
  
  chunkid_ = value;
  // @@protoc_insertion_point(field_set:rsync.FileChunkInfo.chunkId)
}

// int32 checksum1 = 2;
inline void FileChunkInfo::clear_checksum1() {
  checksum1_ = 0;
}
inline ::google::protobuf::int32 FileChunkInfo::checksum1() const {
  // @@protoc_insertion_point(field_get:rsync.FileChunkInfo.checksum1)
  return checksum1_;
}
inline void FileChunkInfo::set_checksum1(::google::protobuf::int32 value) {
  
  checksum1_ = value;
  // @@protoc_insertion_point(field_set:rsync.FileChunkInfo.checksum1)
}

// string checksum2 = 3;
inline void FileChunkInfo::clear_checksum2() {
  checksum2_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& FileChunkInfo::checksum2() const {
  // @@protoc_insertion_point(field_get:rsync.FileChunkInfo.checksum2)
  return checksum2_.GetNoArena();
}
inline void FileChunkInfo::set_checksum2(const ::std::string& value) {
  
  checksum2_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:rsync.FileChunkInfo.checksum2)
}
#if LANG_CXX11
inline void FileChunkInfo::set_checksum2(::std::string&& value) {
  
  checksum2_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:rsync.FileChunkInfo.checksum2)
}
#endif
inline void FileChunkInfo::set_checksum2(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  
  checksum2_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:rsync.FileChunkInfo.checksum2)
}
inline void FileChunkInfo::set_checksum2(const char* value, size_t size) {
  
  checksum2_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:rsync.FileChunkInfo.checksum2)
}
inline ::std::string* FileChunkInfo::mutable_checksum2() {
  
  // @@protoc_insertion_point(field_mutable:rsync.FileChunkInfo.checksum2)
  return checksum2_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* FileChunkInfo::release_checksum2() {
  // @@protoc_insertion_point(field_release:rsync.FileChunkInfo.checksum2)
  
  return checksum2_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void FileChunkInfo::set_allocated_checksum2(::std::string* checksum2) {
  if (checksum2 != nullptr) {
    
  } else {
    
  }
  checksum2_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), checksum2);
  // @@protoc_insertion_point(field_set_allocated:rsync.FileChunkInfo.checksum2)
}

// -------------------------------------------------------------------

// RsyncReply

// bool success = 1;
inline void RsyncReply::clear_success() {
  success_ = false;
}
inline bool RsyncReply::success() const {
  // @@protoc_insertion_point(field_get:rsync.RsyncReply.success)
  return success_;
}
inline void RsyncReply::set_success(bool value) {
  
  success_ = value;
  // @@protoc_insertion_point(field_set:rsync.RsyncReply.success)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace rsync

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // PROTOBUF_INCLUDED_rsync_2eproto
