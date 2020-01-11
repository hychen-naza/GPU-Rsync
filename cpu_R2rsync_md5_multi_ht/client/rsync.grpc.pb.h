// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: rsync.proto
#ifndef GRPC_rsync_2eproto__INCLUDED
#define GRPC_rsync_2eproto__INCLUDED

#include "rsync.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace rsync {

class Rsync final {
 public:
  static constexpr char const* service_full_name() {
    return "rsync.Rsync";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    // Get product bid
    virtual ::grpc::Status PreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::rsync::RsyncReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>> AsyncPreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>>(AsyncPreCalcuRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>> PrepareAsyncPreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>>(PrepareAsyncPreCalcuRaw(context, request, cq));
    }
    virtual ::grpc::Status CalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::rsync::RsyncReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>> AsyncCalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>>(AsyncCalcuDiffRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>> PrepareAsyncCalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>>(PrepareAsyncCalcuDiffRaw(context, request, cq));
    }
    class experimental_async_interface {
     public:
      virtual ~experimental_async_interface() {}
      // Get product bid
      virtual void PreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void PreCalcu(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void CalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) = 0;
      virtual void CalcuDiff(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) = 0;
    };
    virtual class experimental_async_interface* experimental_async() { return nullptr; }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>* AsyncPreCalcuRaw(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>* PrepareAsyncPreCalcuRaw(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>* AsyncCalcuDiffRaw(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::rsync::RsyncReply>* PrepareAsyncCalcuDiffRaw(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status PreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::rsync::RsyncReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>> AsyncPreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>>(AsyncPreCalcuRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>> PrepareAsyncPreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>>(PrepareAsyncPreCalcuRaw(context, request, cq));
    }
    ::grpc::Status CalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::rsync::RsyncReply* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>> AsyncCalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>>(AsyncCalcuDiffRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>> PrepareAsyncCalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>>(PrepareAsyncCalcuDiffRaw(context, request, cq));
    }
    class experimental_async final :
      public StubInterface::experimental_async_interface {
     public:
      void PreCalcu(::grpc::ClientContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) override;
      void PreCalcu(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) override;
      void CalcuDiff(::grpc::ClientContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) override;
      void CalcuDiff(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::rsync::RsyncReply* response, std::function<void(::grpc::Status)>) override;
     private:
      friend class Stub;
      explicit experimental_async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class experimental_async_interface* experimental_async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class experimental_async async_stub_{this};
    ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>* AsyncPreCalcuRaw(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>* PrepareAsyncPreCalcuRaw(::grpc::ClientContext* context, const ::rsync::FileHead& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>* AsyncCalcuDiffRaw(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::rsync::RsyncReply>* PrepareAsyncCalcuDiffRaw(::grpc::ClientContext* context, const ::rsync::FileInfo& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_PreCalcu_;
    const ::grpc::internal::RpcMethod rpcmethod_CalcuDiff_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    // Get product bid
    virtual ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response);
    virtual ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_PreCalcu : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_PreCalcu() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_PreCalcu() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPreCalcu(::grpc::ServerContext* context, ::rsync::FileHead* request, ::grpc::ServerAsyncResponseWriter< ::rsync::RsyncReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_CalcuDiff : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_CalcuDiff() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_CalcuDiff() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestCalcuDiff(::grpc::ServerContext* context, ::rsync::FileInfo* request, ::grpc::ServerAsyncResponseWriter< ::rsync::RsyncReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_PreCalcu<WithAsyncMethod_CalcuDiff<Service > > AsyncService;
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_PreCalcu : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    ExperimentalWithCallbackMethod_PreCalcu() {
      ::grpc::Service::experimental().MarkMethodCallback(0,
        new ::grpc::internal::CallbackUnaryHandler< ::rsync::FileHead, ::rsync::RsyncReply>(
          [this](::grpc::ServerContext* context,
                 const ::rsync::FileHead* request,
                 ::rsync::RsyncReply* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   return this->PreCalcu(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithCallbackMethod_PreCalcu() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class ExperimentalWithCallbackMethod_CalcuDiff : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    ExperimentalWithCallbackMethod_CalcuDiff() {
      ::grpc::Service::experimental().MarkMethodCallback(1,
        new ::grpc::internal::CallbackUnaryHandler< ::rsync::FileInfo, ::rsync::RsyncReply>(
          [this](::grpc::ServerContext* context,
                 const ::rsync::FileInfo* request,
                 ::rsync::RsyncReply* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   return this->CalcuDiff(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithCallbackMethod_CalcuDiff() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  typedef ExperimentalWithCallbackMethod_PreCalcu<ExperimentalWithCallbackMethod_CalcuDiff<Service > > ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_PreCalcu : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_PreCalcu() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_PreCalcu() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_CalcuDiff : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_CalcuDiff() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_CalcuDiff() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_PreCalcu : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithRawMethod_PreCalcu() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_PreCalcu() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPreCalcu(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_CalcuDiff : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithRawMethod_CalcuDiff() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_CalcuDiff() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestCalcuDiff(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_PreCalcu : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    ExperimentalWithRawCallbackMethod_PreCalcu() {
      ::grpc::Service::experimental().MarkMethodRawCallback(0,
        new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
          [this](::grpc::ServerContext* context,
                 const ::grpc::ByteBuffer* request,
                 ::grpc::ByteBuffer* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->PreCalcu(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithRawCallbackMethod_PreCalcu() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void PreCalcu(::grpc::ServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class ExperimentalWithRawCallbackMethod_CalcuDiff : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    ExperimentalWithRawCallbackMethod_CalcuDiff() {
      ::grpc::Service::experimental().MarkMethodRawCallback(1,
        new ::grpc::internal::CallbackUnaryHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
          [this](::grpc::ServerContext* context,
                 const ::grpc::ByteBuffer* request,
                 ::grpc::ByteBuffer* response,
                 ::grpc::experimental::ServerCallbackRpcController* controller) {
                   this->CalcuDiff(context, request, response, controller);
                 }));
    }
    ~ExperimentalWithRawCallbackMethod_CalcuDiff() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual void CalcuDiff(::grpc::ServerContext* context, const ::grpc::ByteBuffer* request, ::grpc::ByteBuffer* response, ::grpc::experimental::ServerCallbackRpcController* controller) { controller->Finish(::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "")); }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_PreCalcu : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_PreCalcu() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::rsync::FileHead, ::rsync::RsyncReply>(std::bind(&WithStreamedUnaryMethod_PreCalcu<BaseClass>::StreamedPreCalcu, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_PreCalcu() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status PreCalcu(::grpc::ServerContext* context, const ::rsync::FileHead* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedPreCalcu(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::rsync::FileHead,::rsync::RsyncReply>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_CalcuDiff : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_CalcuDiff() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::internal::StreamedUnaryHandler< ::rsync::FileInfo, ::rsync::RsyncReply>(std::bind(&WithStreamedUnaryMethod_CalcuDiff<BaseClass>::StreamedCalcuDiff, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_CalcuDiff() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status CalcuDiff(::grpc::ServerContext* context, const ::rsync::FileInfo* request, ::rsync::RsyncReply* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedCalcuDiff(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::rsync::FileInfo,::rsync::RsyncReply>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_PreCalcu<WithStreamedUnaryMethod_CalcuDiff<Service > > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_PreCalcu<WithStreamedUnaryMethod_CalcuDiff<Service > > StreamedService;
};

}  // namespace rsync


#endif  // GRPC_rsync_2eproto__INCLUDED