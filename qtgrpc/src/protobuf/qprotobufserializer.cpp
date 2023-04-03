// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Alexey Edelev <semlanik@gmail.com>, Viktor Kopp <vifactor@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprotobufserializer.h"
#include "qprotobufserializer_p.h"

#include "qtprotobuftypes.h"

#include <QtCore/qmetatype.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qreadwritelock.h>

#include <QtProtobuf/private/qprotobufserializer_p.h>
#include <QtProtobuf/private/qprotobufmessage_p.h>


QT_BEGIN_NAMESPACE

namespace {

/*
    \internal
    \brief The HandlersRegistry is a container to store mapping between metatype
    identifier and serialization handlers.
*/
struct HandlersRegistry
{
    void registerHandler(QMetaType type, const QtProtobufPrivate::SerializationHandler &handlers)
    {
        QWriteLocker locker(&m_lock);
        m_registry[type] = handlers;
    }

    QtProtobufPrivate::SerializationHandler findHandler(QMetaType type)
    {
        QtProtobufPrivate::SerializationHandler handler;
        QReadLocker locker(&m_lock);
        auto it = m_registry.constFind(type);
        if (it != m_registry.constEnd())
            handler = it.value();
        return handler;
    }

private:
    QReadWriteLock m_lock;
    QHash<QMetaType, QtProtobufPrivate::SerializationHandler> m_registry;
};
Q_GLOBAL_STATIC(HandlersRegistry, handlersRegistry)
} // namespace

void QtProtobufPrivate::registerHandler(QMetaType type,
                                        const QtProtobufPrivate::SerializationHandler &handlers)
{
    handlersRegistry->registerHandler(type, handlers);
}

QtProtobufPrivate::SerializationHandler QtProtobufPrivate::findHandler(QMetaType type)
{
    if (!handlersRegistry.exists())
        return {};
    return handlersRegistry->findHandler(type);
}

/*!
    \class QProtobufSerializer
    \inmodule QtProtobuf
    \since 6.5
    \brief The QProtobufSerializer class is interface that represents
           basic functions for serialization/deserialization.
    \reentrant

    The QProtobufSerializer class registers serializers/deserializers for
    classes implementing a protobuf message, inheriting QProtobufMessage. These
    classes are generated automatically, based on a .proto file, using the cmake
    build macro qt6_add_protobuf or by running qtprotobufgen directly.
*/

/*!
    \fn QProtobufSerializer::DeserializationError QProtobufSerializer::deserializationError() const

    Returns the last deserialization error.
*/

/*!
    \fn QString QProtobufSerializer::deserializationErrorString() const

    Returns a human-readable string describing the last deserialization error.
    If there was no error, an empty string is returned.
*/

/*!
    \fn template<typename T> inline void qRegisterProtobufType()

    Registers a Protobuf type \a T.
    This function is normally called by generated code.
*/

/*!
    \fn template<typename K, typename V> inline void qRegisterProtobufMapType();

    Registers a Protobuf map type \c K and \c V.
    \c V must be a QProtobufMessage.
    This function is normally called by generated code.
*/

/*!
    \fn template<typename T> inline void qRegisterProtobufEnumType();

    Registers serializers for enumeration type \c T in QtProtobuf global
    serializers registry.

    This function is normally called by generated code.
*/

using namespace Qt::StringLiterals;
using namespace QtProtobufPrivate;

template<>
Q_REQUIRED_RESULT
QByteArray QProtobufSerializerPrivate::serializeListType<QByteArray>(const QByteArrayList &listValue, int &outFieldIndex)
{
    qProtoDebug("listValue.count %" PRIdQSIZETYPE " outFieldIndex %d", listValue.count(),
                outFieldIndex);

    if (listValue.isEmpty()) {
        outFieldIndex = QtProtobufPrivate::NotUsedFieldIndex;
        return QByteArray();
    }

    QByteArray serializedList;
    for (const QByteArray &value : listValue) {
        serializedList.append(QProtobufSerializerPrivate::encodeHeader(
                outFieldIndex, QtProtobuf::WireTypes::LengthDelimited));
        serializedList.append(serializeLengthDelimited(value, false));
    }

    outFieldIndex = QtProtobufPrivate::NotUsedFieldIndex;
    return serializedList;
}

template<>
Q_REQUIRED_RESULT
bool QProtobufSerializerPrivate::deserializeList<QByteArray>(QProtobufSelfcheckIterator &it, QVariant &previousValue)
{
    qProtoDebug("currentByte: 0x%x", *it);

    std::optional<QByteArray> result = deserializeLengthDelimited(it);
    if (result) {
        QByteArrayList list = previousValue.value<QByteArrayList>();
        list.append(std::move(*result));
        previousValue.setValue(list);
        return true;
    }
    return false;
}

template<>
Q_REQUIRED_RESULT
bool QProtobufSerializerPrivate::deserializeList<QString>(QProtobufSelfcheckIterator &it, QVariant &previousValue)
{
    qProtoDebug("currentByte: 0x%x", *it);

    std::optional<QByteArray> result = deserializeLengthDelimited(it);
    if (result) {
        QStringList list = previousValue.value<QStringList>();
        list.append(QString::fromUtf8(*result));
        previousValue.setValue(list);
        return true;
    }
    return false;
}

template<std::size_t N>
using SerializerRegistryType =
        std::array<QProtobufSerializerPrivate::ProtobufSerializationHandler, N>;

namespace {
#define QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(Type, WireType)                                \
    {                                                                                              \
        QMetaType::fromType<Type>(),                                                               \
                QProtobufSerializerPrivate::serializeWrapper<                                      \
                        Type, QProtobufSerializerPrivate::serializeBasic<Type>>,                   \
                QProtobufSerializerPrivate::deserializeBasic<Type>, WireType                       \
    }
#define QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(ListType, Type)                           \
    {                                                                                              \
        QMetaType::fromType<ListType>(),                                                           \
                QProtobufSerializerPrivate::serializeWrapper<                                      \
                        ListType, QProtobufSerializerPrivate::serializeListType<Type>>,            \
                QProtobufSerializerPrivate::deserializeList<Type>,                                 \
                QtProtobuf::WireTypes::LengthDelimited                                             \
    }
constexpr SerializerRegistryType<30> IntegratedTypesSerializers = { {
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(float, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(double, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::int32,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::int64,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::uint32,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::uint64,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sint32,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sint64,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::fixed32,
                                                    QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::fixed64,
                                                    QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sfixed32,
                                                    QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::sfixed64,
                                                    QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QtProtobuf::boolean,
                                                    QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QString,
                                                    QtProtobuf::WireTypes::LengthDelimited),
        QT_CONSTRUCT_PROTOBUF_SERIALIZATION_HANDLER(QByteArray,
                                                    QtProtobuf::WireTypes::LengthDelimited),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::floatList, float),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::doubleList, double),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::int32List, QtProtobuf::int32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::int64List, QtProtobuf::int64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::uint32List,
                                                         QtProtobuf::uint32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::uint64List,
                                                         QtProtobuf::uint64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sint32List,
                                                         QtProtobuf::sint32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sint64List,
                                                         QtProtobuf::sint64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::fixed32List,
                                                         QtProtobuf::fixed32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::fixed64List,
                                                         QtProtobuf::fixed64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sfixed32List,
                                                         QtProtobuf::sfixed32),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::sfixed64List,
                                                         QtProtobuf::sfixed64),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QtProtobuf::boolList, QtProtobuf::boolean),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QStringList, QString),
        QT_CONSTRUCT_PROTOBUF_LIST_SERIALIZATION_HANDLER(QByteArrayList, QByteArray),
} };

#define QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(ListType, Type, WireType)      \
  {                                                                                                \
    QMetaType::fromType<ListType>(),                                                               \
            QProtobufSerializerPrivate::serializeWrapper<                                          \
                    ListType,                                                                      \
                    QProtobufSerializerPrivate::serializeNonPackedListTypeCommon<Type, WireType>>, \
            QProtobufSerializerPrivate::deserializeNonPackedList<Type>, WireType                   \
  }
constexpr SerializerRegistryType<13> IntegratedNonPackedSerializers = { {
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
            QtProtobuf::floatList, float, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
            QtProtobuf::doubleList, double, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::int32List, QtProtobuf::int32, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::int64List, QtProtobuf::int64, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::uint32List, QtProtobuf::uint32, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::uint64List, QtProtobuf::uint64, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sint32List, QtProtobuf::sint32, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sint64List, QtProtobuf::sint64, QtProtobuf::WireTypes::Varint),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::fixed32List, QtProtobuf::fixed32, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::fixed64List, QtProtobuf::fixed64, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sfixed32List, QtProtobuf::sfixed32, QtProtobuf::WireTypes::Fixed32),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::sfixed64List, QtProtobuf::sfixed64, QtProtobuf::WireTypes::Fixed64),
        QT_CONSTRUCT_PROTOBUF_NON_PACKED_LIST_SERIALIZATION_HANDLER(
                QtProtobuf::boolList, QtProtobuf::boolean, QtProtobuf::WireTypes::Varint),
} };

template<std::size_t N>
std::optional<QProtobufSerializerPrivate::ProtobufSerializationHandler>
findIntegratedTypeHandlerImpl(QMetaType metaType, const SerializerRegistryType<N> &registry)
{
    typename SerializerRegistryType<N>::const_iterator it = std::find_if(
            registry.begin(), registry.end(),
            [&metaType](const QProtobufSerializerPrivate::ProtobufSerializationHandler &handler) {
                return handler.metaType == metaType;
            });
    if (it == registry.end())
        return std::nullopt;
    return { *it };
}

std::optional<QProtobufSerializerPrivate::ProtobufSerializationHandler>
findIntegratedTypeHandler(QMetaType metaType, bool nonPacked)
{
    if (nonPacked)
        return findIntegratedTypeHandlerImpl(metaType, IntegratedNonPackedSerializers);

    return findIntegratedTypeHandlerImpl(metaType, IntegratedTypesSerializers);
}
}

/*!
    Constructs a new serializer instance.
*/
QProtobufSerializer::QProtobufSerializer() : d_ptr(new QProtobufSerializerPrivate(this))
{
}

/*!
    Destroys the serializer instance.
*/
QProtobufSerializer::~QProtobufSerializer() = default;

/*!
    This is called by serialize() to serialize a registered Protobuf message
    \a message with \a ordering. \a message must not be
    \nullptr.
    Returns a QByteArray containing the serialized message.
*/
QByteArray QProtobufSerializer::serializeMessage(
        const QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering) const
{
    QByteArray result;
    for (int index = 0; index < ordering.fieldCount(); ++index) {
        int propertyIndex =
                ordering.getPropertyIndex(index) + message->metaObject()->propertyOffset();
        int fieldIndex = ordering.getFieldNumber(index);
        Q_ASSERT_X(fieldIndex < 536870912 && fieldIndex > 0, "", "fieldIndex is out of range");
        QMetaProperty metaProperty = message->metaObject()->property(propertyIndex);
        QVariant propertyValue = metaProperty.readOnGadget(message);
        result.append(d_ptr->serializeProperty(propertyValue,
                                               QProtobufPropertyOrderingInfo(ordering, index)));
    }

    // Restore any unknown fields we have stored away:
    const QProtobufMessagePrivate *messagePrivate = QProtobufMessagePrivate::get(message);
    for (const auto &[bytes, occurrences] : messagePrivate->unknownEntries.asKeyValueRange())
        result += bytes.repeated(occurrences);

    return result;
}

void QProtobufSerializerPrivate::setUnexpectedEndOfStreamError()
{
    setDeserializationError(QAbstractProtobufSerializer::UnexpectedEndOfStreamError,
                            QCoreApplication::translate("QtProtobuf", "Unexpected end of stream"));
}

void QProtobufSerializerPrivate::clearError()
{
    deserializationError = QAbstractProtobufSerializer::NoError;
    deserializationErrorString.clear();
}

/*!
    This is called by deserialize() to deserialize a registered Protobuf message
    \a message with \a ordering, from a QByteArrayView \a data. \a message
    can be assumed to not be \nullptr.
    Returns \c true if deserialization was successful, otherwise \c false.
*/
bool QProtobufSerializer::deserializeMessage(
        QProtobufMessage *message, const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QByteArrayView data) const
{
    d_ptr->clearError();
    QProtobufSelfcheckIterator it = QProtobufSelfcheckIterator::fromView(data);
    while (it.isValid() && it != data.end()) {
        if (!d_ptr->deserializeProperty(message, ordering, it))
            return false;
    }
    if (!it.isValid())
        d_ptr->setUnexpectedEndOfStreamError();
    return it.isValid();
}

/*!
    Serialize an \a message with \a ordering and \a fieldInfo.
    Returns a QByteArray containing the serialized message.

    You should not call this function directly.
*/
QByteArray
QProtobufSerializer::serializeObject(const QProtobufMessage *message,
                                     const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
                                     const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    QByteArray result = QProtobufSerializerPrivate::encodeHeader(
            fieldInfo.getFieldNumber(), QtProtobuf::WireTypes::LengthDelimited);
    result.append(QProtobufSerializerPrivate::prependLengthDelimitedSize(
            serializeMessage(message, ordering)));
    return result;
}

/*!
    Deserialize an \a message with \a ordering from a QProtobufSelfcheckIterator
    \a it. Returns \c true if deserialization was successful, otherwise
    \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeObject(QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QProtobufSelfcheckIterator &it) const
{
    if (it.bytesLeft() == 0) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    std::optional<QByteArray> array = QProtobufSerializerPrivate::deserializeLengthDelimited(it);
    if (!array) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    return deserializeMessage(message, ordering, array.value());
}

/*!
    This function is called to serialize \a message as a part of list property
    with \a ordering and \a fieldInfo.

    You should not call this function directly.
*/
QByteArray QProtobufSerializer::serializeListObject(
        const QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    return serializeObject(message, ordering, fieldInfo);
}

/*!
    This function deserializes an \a message from byte stream as part of list property, with
    \a ordering from a QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeListObject(QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QProtobufSelfcheckIterator &it) const
{
    return deserializeObject(message, ordering, it);
}

/*!
    This function serializes QMap pair of \a key and \a value with
    \a fieldInfo to a QByteArray

    You should not call this function directly.
*/
QByteArray
QProtobufSerializer::serializeMapPair(const QVariant &key, const QVariant &value,
                                      const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    QByteArray result = QProtobufSerializerPrivate::encodeHeader(
            fieldInfo.getFieldNumber(), QtProtobuf::WireTypes::LengthDelimited);
    result.append(QProtobufSerializerPrivate::prependLengthDelimitedSize(
            d_ptr->serializeProperty(key, fieldInfo.infoForMapKey())
            + d_ptr->serializeProperty(value, fieldInfo.infoForMapValue())));
    return result;
}

/*!
    This function deserializes QMap pair of \a key and \a value from a
    QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeMapPair(QVariant &key, QVariant &value, QProtobufSelfcheckIterator &it) const
{
    return d_ptr->deserializeMapPair(key, value, it);
}

/*!
    This function serializes \a value as a QByteArray for enum associated with
    property \a fieldInfo.

    You should not call this function directly.
*/
QByteArray QProtobufSerializer::serializeEnum(QtProtobuf::int64 value,
                                              const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    QtProtobuf::WireTypes type = QtProtobuf::WireTypes::Varint;
    int fieldIndex = fieldInfo.getFieldNumber();
    QByteArray result = QProtobufSerializerPrivate::serializeBasic<QtProtobuf::int64>(value, fieldIndex);
    if (fieldIndex != QtProtobufPrivate::NotUsedFieldIndex)
        result.prepend(QProtobufSerializerPrivate::encodeHeader(fieldIndex, type));
    return result;
}

/*!
    This function serializes a list, \a value, as a QByteArray for enum
    associated with property \a fieldInfo.

    You should not call this function directly.
*/
QByteArray
QProtobufSerializer::serializeEnumList(const QList<QtProtobuf::int64> &value,
                                       const QProtobufPropertyOrderingInfo &fieldInfo) const
{
    QtProtobuf::WireTypes type = QtProtobuf::WireTypes::LengthDelimited;
    int fieldIndex = fieldInfo.getFieldNumber();
    QByteArray result =
            QProtobufSerializerPrivate::serializeListType<QtProtobuf::int64>(value, fieldIndex);
    if (fieldIndex != QtProtobufPrivate::NotUsedFieldIndex)
        result.prepend(QProtobufSerializerPrivate::encodeHeader(fieldIndex, type));
    return result;
}

/*!
    This function deserializes an enum \a value from a QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeEnum(QtProtobuf::int64 &value, QProtobufSelfcheckIterator &it) const
{
    QVariant variantValue;
    if (!QProtobufSerializerPrivate::deserializeBasic<QtProtobuf::int64>(it, variantValue)) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    value = variantValue.value<QtProtobuf::int64>();
    return true;
}

/*!
    This function deserializes a list of enum \a value from a QProtobufSelfcheckIterator \a it.
    Returns \c true if deserialization was successful, otherwise \c false.

    You should not call this function directly.
*/
bool QProtobufSerializer::deserializeEnumList(QList<QtProtobuf::int64> &value, QProtobufSelfcheckIterator &it) const
{
    QVariant variantValue;
    if (!QProtobufSerializerPrivate::deserializeList<QtProtobuf::int64>(it, variantValue)) {
        d_ptr->setUnexpectedEndOfStreamError();
        return false;
    }
    value = variantValue.value<QList<QtProtobuf::int64>>();
    return true;
}

QProtobufSerializerPrivate::QProtobufSerializerPrivate(QProtobufSerializer *q) : q_ptr(q)
{
}

void QProtobufSerializerPrivate::skipVarint(QProtobufSelfcheckIterator &it)
{
    while ((*it) & 0x80)
        ++it;
    ++it;
}

void QProtobufSerializerPrivate::skipLengthDelimited(QProtobufSelfcheckIterator &it)
{
    //Get length of length-delimited field
    auto opt = QProtobufSerializerPrivate::deserializeVarintCommon<QtProtobuf::uint64>(it);
    if (!opt) {
        it += it.bytesLeft();
        return;
    }
    QtProtobuf::uint64 length = opt.value();
    it += length;
}

qsizetype QProtobufSerializerPrivate::skipSerializedFieldBytes(QProtobufSelfcheckIterator &it, QtProtobuf::WireTypes type)
{
    const auto *initialIt = QByteArray::const_iterator(it);
    switch (type) {
    case QtProtobuf::WireTypes::Varint:
        skipVarint(it);
        break;
    case QtProtobuf::WireTypes::Fixed32:
        it += sizeof(decltype(QtProtobuf::fixed32::_t));
        break;
    case QtProtobuf::WireTypes::Fixed64:
        it += sizeof(decltype(QtProtobuf::fixed64::_t));
        break;
    case QtProtobuf::WireTypes::LengthDelimited:
        skipLengthDelimited(it);
        break;
    case QtProtobuf::WireTypes::Unknown:
    default:
        Q_UNREACHABLE();
        return 0;
    }

    return std::distance(initialIt, QByteArray::const_iterator(it));
}

QByteArray
QProtobufSerializerPrivate::serializeProperty(const QVariant &propertyValue,
                                              const QProtobufPropertyOrderingInfo &fieldInfo)
{
    QtProtobuf::WireTypes type = QtProtobuf::WireTypes::Unknown;
    QMetaType metaType = propertyValue.metaType();

    qProtoDebug() << "propertyValue" << propertyValue << "fieldIndex" << fieldInfo.getFieldNumber()
                  << "metaType" << metaType.name();

    if (metaType.id() == QMetaType::UnknownType || propertyValue.isNull()) {
        // Empty value
        return {};
    }

    QByteArray result;
    //TODO: replace with some common function
    auto basicHandler = findIntegratedTypeHandler(
            metaType, fieldInfo.getFieldFlags() & QtProtobufPrivate::NonPacked);
    if (basicHandler) {
        type = basicHandler->wireType;
        int fieldIndex = fieldInfo.getFieldNumber();
        result.append(basicHandler->serializer(propertyValue, fieldIndex));
        if (fieldIndex != QtProtobufPrivate::NotUsedFieldIndex
                && type != QtProtobuf::WireTypes::Unknown) {
            result.prepend(QProtobufSerializerPrivate::encodeHeader(fieldIndex, type));
        }
    } else {
        auto handler = QtProtobufPrivate::findHandler(metaType);
        if (!handler.serializer) {
            qProtoWarning() << "No serializer for type" << propertyValue.typeName();
            return result;
        }
        handler.serializer(q_ptr, propertyValue, fieldInfo, result);
    }
    return result;
}

bool QProtobufSerializerPrivate::deserializeProperty(
        QProtobufMessage *message,
        const QtProtobufPrivate::QProtobufPropertyOrdering &ordering,
        QProtobufSelfcheckIterator &it)
{
    Q_ASSERT(it.isValid() && it.bytesLeft() > 0);
    //Each iteration we expect iterator is setup to beginning of next chunk
    int fieldNumber = QtProtobufPrivate::NotUsedFieldIndex;
    QtProtobuf::WireTypes wireType = QtProtobuf::WireTypes::Unknown;
    const QProtobufSelfcheckIterator itBeforeHeader = it; // copy this, we may need it later
    if (!QProtobufSerializerPrivate::decodeHeader(it, fieldNumber, wireType)) {
        setDeserializationError(
                QAbstractProtobufSerializer::InvalidHeaderError,
                QCoreApplication::translate("QtProtobuf",
                                     "Message received doesn't contain valid header byte."));
        return false;
    }

    int index = ordering.indexOfFieldNumber(fieldNumber);
    if (index == -1) {
        // This is an unknown field, it may have been added in a later revision
        // of the Message we are currently deserializing. We must store the
        // bytes for this field and re-emit them later if this message is
        // serialized again.
        qsizetype length = std::distance(itBeforeHeader, it); // size of header
        length += QProtobufSerializerPrivate::skipSerializedFieldBytes(it, wireType);

        if (!it.isValid()) {
            setUnexpectedEndOfStreamError();
            return false;
        }

        QProtobufMessagePrivate *messagePrivate = QProtobufMessagePrivate::get(message);
        messagePrivate->storeUnknownEntry(QByteArrayView(itBeforeHeader.data(), length));
        return true;
    }

    int propertyIndex = ordering.getPropertyIndex(index) + message->metaObject()->propertyOffset();
    QMetaProperty metaProperty = message->metaObject()->property(propertyIndex);

    qProtoDebug() << "wireType:" << wireType << "metaProperty:" << metaProperty.typeName()
                  << "currentByte:" << QString::number((*it), 16);

    QVariant newPropertyValue;
    newPropertyValue = metaProperty.readOnGadget(message);
    QMetaType metaType = metaProperty.metaType();

    //TODO: replace with some common function
    auto basicHandler = findIntegratedTypeHandler(
            metaType, ordering.getFieldFlags(index) & QtProtobufPrivate::NonPacked);
    if (basicHandler) {
        if (!basicHandler->deserializer(it, newPropertyValue)) {
            setUnexpectedEndOfStreamError();
            return false;
        }
    } else {
        auto handler = QtProtobufPrivate::findHandler(metaType);
        if (!handler.deserializer) {
            qProtoWarning() << "No deserializer for type" << metaProperty.typeName();
            setDeserializationError(
                    QAbstractProtobufSerializer::NoDeserializerError,
                    QCoreApplication::translate("QtProtobuf",
                                         "No deserializer is registered for type %1"));
            return false;
        }
        handler.deserializer(q_ptr, it, newPropertyValue);
    }

    metaProperty.writeOnGadget(message, newPropertyValue);
    return true;
}

bool QProtobufSerializerPrivate::deserializeMapPair(QVariant &key, QVariant &value, QProtobufSelfcheckIterator &it)
{
    int mapIndex = 0;
    QtProtobuf::WireTypes type = QtProtobuf::WireTypes::Unknown;
    auto opt = QProtobufSerializerPrivate::deserializeVarintCommon<QtProtobuf::uint32>(it);
    if (!opt) {
        setUnexpectedEndOfStreamError();
        return false;
    }
    unsigned int count = opt.value();
    qProtoDebug("count: %u", count);
    QProtobufSelfcheckIterator last = it + count;
    while (it.isValid() && it != last) {
        if (!QProtobufSerializerPrivate::decodeHeader(it, mapIndex, type)) {
            setDeserializationError(
                        QAbstractProtobufSerializer::InvalidHeaderError,
                        QCoreApplication::translate("QtProtobuf",
                                             "Message received doesn't contain valid header byte."));
            return false;
        }
        if (mapIndex == 1) {
            //Only simple types are supported as keys
            QMetaType metaType = key.metaType();
            auto basicHandler = findIntegratedTypeHandler(metaType, false);
            if (!basicHandler) {
                // clang-format off
                QString errorStr = QCoreApplication::translate("QtProtobuf",
                                                        "Either there is no deserializer for type "
                                                        "%1 or it is not a builtin type")
                        .arg(QLatin1String(key.typeName()));
                // clang-format on
                setDeserializationError(QAbstractProtobufSerializer::NoDeserializerError, errorStr);
                return false;
            }
            basicHandler->deserializer(it, key);
        } else {
            //TODO: replace with some common function
            QMetaType metaType = value.metaType();
            auto basicHandler = findIntegratedTypeHandler(metaType, false);
            if (basicHandler) {
                basicHandler->deserializer(it, value);
            } else {
                auto handler = QtProtobufPrivate::findHandler(metaType);
                if (!handler.deserializer) {
                    qProtoWarning() << "No deserializer for type" << value.typeName();
                    setDeserializationError(
                                QAbstractProtobufSerializer::NoDeserializerError,
                                QCoreApplication::translate("QtProtobuf",
                                                     "No deserializer is registered for type %1")
                                .arg(QLatin1String(value.typeName())));
                    return false;
                }
                handler.deserializer(q_ptr, it, value);
            }
        }
    }
    return it == last;
}

QAbstractProtobufSerializer::DeserializationError QProtobufSerializer::deserializationError() const
{
    return d_ptr->deserializationError;
}

QString QProtobufSerializer::deserializationErrorString() const
{
    return d_ptr->deserializationErrorString;
}

void QProtobufSerializerPrivate::setDeserializationError(
        QAbstractProtobufSerializer::DeserializationError error, const QString &errorString)
{
    deserializationError = error;
    deserializationErrorString = errorString;
}

QT_END_NAMESPACE
