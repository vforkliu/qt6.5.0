// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qgrpcgenerator.h"
#include "clientdeclarationprinter.h"
#include "clientdefinitionprinter.h"
#include "serverdeclarationprinter.h"

#include "grpctemplates.h"
#include "utils.h"
#include "options.h"

#include <set>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.h>

using namespace ::QtGrpc;
using namespace ::qtprotoccommon;
using namespace ::google::protobuf;
using namespace ::google::protobuf::io;
using namespace ::google::protobuf::compiler;

QGrpcGenerator::QGrpcGenerator() : GeneratorBase()
{}

QGrpcGenerator::~QGrpcGenerator() = default;

bool QGrpcGenerator::Generate(const FileDescriptor *file,
                              [[maybe_unused]] const std::string &parameter,
                              GeneratorContext *generatorContext,
                              std::string *error) const
{
    assert(file != nullptr && generatorContext != nullptr);
    if (file->syntax() != FileDescriptor::SYNTAX_PROTO3) {
        *error = "Invalid proto used. qtgrpcgen only supports 'proto3' syntax";
        return false;
    }

    return GenerateClientServices(file, generatorContext);
}

std::set<std::string> QGrpcGenerator::GetInternalIncludes(const FileDescriptor *file)
{
    std::set<std::string> includes;
    assert(file != nullptr);
    for (int i = 0; i < file->service_count(); ++i) {
        const ServiceDescriptor *service = file->service(i);
        for (int i = 0; i < service->method_count(); ++i) {
            const MethodDescriptor *method = service->method(i);
            if (method->input_type()->file() != service->file()) {
                includes.insert(utils::removeFileSuffix(method->input_type()->file()->name())
                                + CommonTemplates::ProtoFileSuffix());
            }

            if (method->output_type()->file() != service->file()) {
                includes.insert(utils::removeFileSuffix(method->output_type()->file()->name())
                                + CommonTemplates::ProtoFileSuffix());
            }
        }
    }
    if (file->message_type_count() > 0) {
        includes.insert(generateBaseName(file, utils::extractFileBasename(file->name()))
                        + CommonTemplates::ProtoFileSuffix());
    }
    return includes;
}

template <typename ServicePrinterT>
void QGrpcGenerator::RunPrinter(const FileDescriptor *file, std::shared_ptr<Printer> printer)
{
    assert(file != nullptr);
    for (int i = 0; i < file->service_count(); ++i) {
        const ServiceDescriptor *service = file->service(i);

        ServicePrinterT servicePrinter(service, printer);
        servicePrinter.run();
    }
}

bool QGrpcGenerator::GenerateClientServices(const FileDescriptor *file,
                                            GeneratorContext *generatorContext) const
{
    assert(file != nullptr && generatorContext != nullptr);
    if (file->service_count() <= 0)
        return true;

    const std::string filename = utils::extractFileBasename(file->name());
    const std::string basename = generateBaseName(file, filename);
    const std::string clientFileName = basename
            + GrpcTemplates::GrpcClientFileSuffix() + CommonTemplates::ProtoFileSuffix();
    std::unique_ptr<ZeroCopyOutputStream> clientHeaderStream(
                generatorContext->Open(clientFileName + ".h"));
    std::unique_ptr<ZeroCopyOutputStream> clientSourceStream(
                generatorContext->Open(clientFileName + ".cpp"));

    std::shared_ptr<Printer> clientHeaderPrinter(new Printer(clientHeaderStream.get(), '$'));
    std::shared_ptr<Printer> clientSourcePrinter(new Printer(clientSourceStream.get(), '$'));

    printDisclaimer(clientHeaderPrinter.get());
    clientHeaderPrinter->Print({ { "filename", filename + "_client" } },
                               CommonTemplates::PreambleTemplate());

    clientHeaderPrinter->Print(CommonTemplates::DefaultProtobufIncludesTemplate());
    if (Options::instance().hasQml())
        clientHeaderPrinter->Print(CommonTemplates::QmlProtobufIncludesTemplate());

    printDisclaimer(clientSourcePrinter.get());
    clientSourcePrinter->Print({ { "include", clientFileName } },
                               CommonTemplates::InternalIncludeTemplate());

    std::set<std::string> externalIncludes
            = {"QAbstractGrpcClient", "QGrpcCallReply", "QGrpcStream"};

    for (const auto &include : externalIncludes) {
        clientHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    std::set<std::string> internalIncludes = QGrpcGenerator::GetInternalIncludes(file);
    for (const auto &include : internalIncludes) {
        clientHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::InternalIncludeTemplate());
    }

    if (Options::instance().hasQml()) {
        clientSourcePrinter->Print({ { "include", "QQmlEngine" } },
                                   CommonTemplates::ExternalIncludeTemplate());
        clientSourcePrinter->Print({ { "include", "QJSEngine" } },
                                   CommonTemplates::ExternalIncludeTemplate());
        clientSourcePrinter->Print({ { "include", "QJSValue" } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    clientHeaderPrinter->PrintRaw("\n");
    if (!Options::instance().exportMacro().empty()) {
        clientHeaderPrinter->Print({ { "export_macro", Options::instance().exportMacro() } },
                                   CommonTemplates::ExportMacroTemplate());
    }

    OpenFileNamespaces(file, clientHeaderPrinter.get());
    OpenFileNamespaces(file, clientSourcePrinter.get());

    QGrpcGenerator::RunPrinter<ClientDeclarationPrinter>(file, clientHeaderPrinter);
    QGrpcGenerator::RunPrinter<ClientDefinitionPrinter>(file, clientSourcePrinter);

    CloseFileNamespaces(file, clientHeaderPrinter.get());
    CloseFileNamespaces(file, clientSourcePrinter.get());

    clientHeaderPrinter->Print({ { "filename", filename + "_client" } },
                               CommonTemplates::FooterTemplate());
    return true;
}

bool QGrpcGenerator::GenerateServerServices(const FileDescriptor *file,
                                            GeneratorContext *generatorContext) const
{
    assert(file != nullptr && generatorContext != nullptr);
    if (file->service_count() <= 0)
        return true;

    const std::string filename = utils::extractFileBasename(file->name());
    const std::string basename = generateBaseName(file, filename);
    std::unique_ptr<ZeroCopyOutputStream> serviceHeaderStream(
            generatorContext->Open(basename + GrpcTemplates::GrpcServiceFileSuffix()
                                   + CommonTemplates::ProtoFileSuffix() + ".h"));
    std::shared_ptr<Printer> serverHeaderPrinter(new Printer(serviceHeaderStream.get(), '$'));

    printDisclaimer(serverHeaderPrinter.get());
    serverHeaderPrinter->Print({ { "filename", filename + "_service" } },
                               CommonTemplates::PreambleTemplate());

    serverHeaderPrinter->Print(CommonTemplates::DefaultProtobufIncludesTemplate());
    if (Options::instance().hasQml())
        serverHeaderPrinter->Print(CommonTemplates::QmlProtobufIncludesTemplate());

    std::set<std::string> externalIncludes = { "QAbstractGrpcClient", "QGrpcCallReply",
                                               "QGrpcStream" };

    for (const auto &include : externalIncludes) {
        serverHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::ExternalIncludeTemplate());
    }

    const std::string serviceIncludes("QAbstractGrpcService");
    serverHeaderPrinter->Print({ { "include", serviceIncludes } },
                               CommonTemplates::ExternalIncludeTemplate());

    std::set<std::string> internalIncludes = QGrpcGenerator::GetInternalIncludes(file);
    for (const auto &include : internalIncludes) {
        serverHeaderPrinter->Print({ { "include", include } },
                                   CommonTemplates::InternalIncludeTemplate());
    }

    serverHeaderPrinter->PrintRaw("\n");
    if (!Options::instance().exportMacro().empty()) {
        serverHeaderPrinter->Print({ { "export_macro", Options::instance().exportMacro() } },
                                   CommonTemplates::ExportMacroTemplate());
    }
    OpenFileNamespaces(file, serverHeaderPrinter.get());

    QGrpcGenerator::RunPrinter<ServerDeclarationPrinter>(file, serverHeaderPrinter);

    CloseFileNamespaces(file, serverHeaderPrinter.get());

    serverHeaderPrinter->Print({ { "filename", filename + "_service" } },
                               CommonTemplates::FooterTemplate());
    return true;
}
