#include "error.h"
#include "Logger.h"

Error::Error(QObject *parent) :
    QObject(parent)
{
}

void Error::readExceptions(hash data) {
    if (data.contains(1))
        readUserException(data[1].value<hash>());

    if (data.contains(2))
        readSystemException(data[2].value<hash>());

    if (data.contains(3))
        readNotFoundException(data[3].value<hash>());
}

void Error::readUserException(hash data) {

    if (!data.contains(1)) {
        LOG_ERROR("Empty User Exception");
        return;
    }

    bool ok;
    int errorCode = data[1].toInt(&ok);

    if (!ok) {
        LOG_ERROR("Unknown User Exception");
        return;
    }

    EDAMErrorCode(errorCode);

    QString parameter;
    if (data.contains(2)) {
        parameter = QString::fromUtf8(data[2].toByteArray());
        LOG_INFO(parameter);
    }
}

void Error::readSystemException(hash data) {
    if (!data.contains(1)) {
        LOG_ERROR("Empty User Exception");
        return;
    }

    bool ok;
    int errorCode = data[1].toInt(&ok);

    if (!ok) {
        LOG_ERROR("Unknown User Exception");
        return;
    }

    EDAMErrorCode(errorCode);

    if (data.contains(2)) {
        QString message = data[2].toString();
        LOG_INFO(message);
    }

    if (data.contains(3)) {
        int rateLimitDuration = data[3].toInt();
        LOG_INFO(QString("Rate Limit Duration: %1 s.").arg(rateLimitDuration));
    }
}

void Error::readNotFoundException(hash data) {
    QString e("Not Found Exception");

    if (data.contains(1)) {
        QString identifier = data[1].toString();
        e.append(" Identifier:");
        e.append(identifier);
    }

    if (data.contains(2)) {
        QString key = data[2].toString();
        e.append(" Key:");
        e.append(key);
    }

    LOG_ERROR(e);
}

void Error::EDAMErrorCode(int errorCode) {
    switch (errorCode) {
    case 1:
        LOG_ERROR("UNKNOWN");
        LOG_INFO("No information available about the error");
        return;
    case 2:
        LOG_ERROR("BAD_DATA_FORMAT");
        LOG_INFO("The format of the request data was incorrect");
        return;
    case 3:
        LOG_ERROR("PERMISSION_DENIED");
        LOG_INFO("Not permitted to perform action");
        return;
    case 4:
        LOG_ERROR("INTERNAL_ERROR");
        LOG_INFO("Unexpected problem with the service");
        return;
    case 5:
        LOG_ERROR("DATA_REQUIRED");
        LOG_INFO("A required parameter/field was absent");
        return;
    case 6:
        LOG_ERROR("LIMIT_REACHED");
        LOG_INFO("Operation denied due to data model limit");
        return;
    case 7:
        LOG_ERROR("QUOTA_REACHED");
        LOG_INFO("Operation denied due to user storage limit");
        return;
    case 8:
        LOG_ERROR("INVALID_AUTH");
        LOG_INFO("Username and/or password incorrect");
        return;
    case 9:
        LOG_ERROR("AUTH_EXPIRED");
        LOG_INFO("Authentication token expired");
        return;
    case 10:
        LOG_ERROR("DATA_CONFLICT");
        LOG_INFO("Change denied due to data model conflict");
        return;
    case 11:
        LOG_ERROR("ENML_VALIDATION");
        LOG_INFO("Content of submitted note was malformed");
        return;
    case 12:
        LOG_ERROR("SHARD_UNAVAILABLE");
        LOG_INFO("Service shard with account data is temporarily down");
        return;
    case 13:
        LOG_ERROR("LEN_TOO_SHORT");
        LOG_INFO("Operation denied due to data model limit, where something such as a string length was too short");
        return;
    case 14:
        LOG_ERROR("LEN_TOO_LONG");
        LOG_INFO("Operation denied due to data model limit, where something such as a string length was too long");
        return;
    case 15:
        LOG_ERROR("TOO_FEW");
        LOG_INFO("Operation denied due to data model limit, where there were too few of something.");
        return;
    case 16:
        LOG_ERROR("TOO_MANY");
        LOG_INFO("Operation denied due to data model limit, where there were too many of something.");
        return;
    case 17:
        LOG_ERROR("UNSUPPORTED_OPERATION");
        LOG_INFO("Operation denied because it is currently unsupported.");
        return;
    case 18:
        LOG_ERROR("TAKEN_DOWN");
        LOG_INFO("Operation denied because access to the corresponding object is prohibited in response to a take-down notice.");
        return;
    case 19:
        LOG_ERROR("RATE_LIMIT_REACHED");
        LOG_INFO("Operation denied because the calling application has reached its hourly API call limit for this user.");
        return;
    default:
        LOG_ERROR("Unknown User Exception.");
        return;
    }
}
