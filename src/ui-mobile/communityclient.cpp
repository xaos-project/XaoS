#include "communityclient.h"

#include <QFile>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUdpSocket>
#include <QTimer>
#include <QUrlQuery>
#include "server_config.h"

static const int DISCOVERY_PORT = 3001;
static const char *EMULATOR_FALLBACK_URL = COMMUNITY_SERVER_URL;

CommunityClient::CommunityClient(QObject *parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) {
  QSettings settings("XaoS", "CommunityClient");
  m_sessionToken = settings.value("sessionToken").toString();
  m_userRole = settings.value("userRole").toString();
  m_displayName = settings.value("displayName").toString();

  // Load saved server URL, or fall back to compile-time default
  QString savedUrl = settings.value("serverUrl").toString();
  if (!savedUrl.isEmpty()) {
    m_serverUrl = savedUrl;
    m_serverFound = true;
  }

  if (!m_sessionToken.isEmpty()) {
      QTimer::singleShot(500, this, [this]() {
          emit authChanged();
          fetchUserRooms();
      });
  }
  startDiscovery();
}


void CommunityClient::startDiscovery() {
  m_discoverySocket = new QUdpSocket(this);
  if (m_discoverySocket->bind(QHostAddress::AnyIPv4, DISCOVERY_PORT,
                               QUdpSocket::ShareAddress |
                                   QUdpSocket::ReuseAddressHint)) {
    connect(m_discoverySocket, &QUdpSocket::readyRead, this,
            &CommunityClient::onDiscoveryDatagram);
    qDebug() << "CommunityClient: Listening for server beacon on UDP port"
             << DISCOVERY_PORT;
  } else {
    qWarning() << "CommunityClient: Failed to bind discovery socket on port"
               << DISCOVERY_PORT << m_discoverySocket->errorString();
  }

  // Only set fallback if no saved URL was loaded
  if (!m_serverFound) {
      m_serverUrl = QString(EMULATOR_FALLBACK_URL);
      m_serverFound = true;
      emit serverUrlChanged();
      emit serverDiscovered();
      qDebug() << "CommunityClient: Initialized with fallback at"
               << m_serverUrl;
  }
}

void CommunityClient::onDiscoveryDatagram() {
  while (m_discoverySocket->hasPendingDatagrams()) {
    QByteArray data;
    data.resize(m_discoverySocket->pendingDatagramSize());
    QHostAddress sender;
    quint16 senderPort;
    m_discoverySocket->readDatagram(data.data(), data.size(), &sender,
                                    &senderPort);

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
      continue;

    QJsonObject obj = doc.object();
    if (obj["service"].toString() != "xaos-community")
      continue;

    QString ip = obj["ip"].toString();
    int port = obj["port"].toInt();
    if (ip.isEmpty() || port == 0)
      continue;

    QString url = QStringLiteral("http://%1:%2").arg(ip).arg(port);

    if (m_serverUrl != url || !m_serverFound) {
      m_serverUrl = url;
      m_serverFound = true;
      emit serverUrlChanged();
      emit serverDiscovered();
      qDebug() << "CommunityClient: Server discovered at" << url;
    }
  }
}


void CommunityClient::setServerUrl(const QString &url) {
  // Abort any in-flight requests so stale replies don't interfere
  m_nam->clearConnectionCache();

  m_serverUrl = url;
  m_serverFound = !url.isEmpty();
  m_loading = false;
  emit loadingChanged();

  QSettings settings("XaoS", "CommunityClient");
  if (url.isEmpty()) {
    settings.remove("serverUrl");
  } else {
    settings.setValue("serverUrl", url);
  }
  emit serverUrlChanged();
}

void CommunityClient::resetToDefaultUrl() {
  setServerUrl(QString(EMULATOR_FALLBACK_URL));
}

void CommunityClient::setLoading(bool loading) {
  if (m_loading != loading) {
    m_loading = loading;
    emit loadingChanged();
  }
}

void CommunityClient::setError(const QString &error) {
  m_error = error;
  emit errorChanged();
  if (!error.isEmpty()) {
    emit networkError(error);
  }
}


void CommunityClient::fetchRoomMembers(int roomId) {
  if (m_serverUrl.isEmpty() || m_sessionToken.isEmpty())
    return;

  QUrl url(m_serverUrl + QStringLiteral("/api/rooms/%1/members").arg(roomId));
  QNetworkRequest request(url);
  request.setRawHeader("Authorization", ("Bearer " + m_sessionToken).toUtf8());

  QNetworkReply *reply = m_nam->get(request);
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onRoomMembersFinished(reply); });
}

void CommunityClient::upload(const QString &title, const QString &author,
                             const QString &xpfData,
                             const QString &thumbnailPath,
                             const QString &formula, int iterations,
                             const QString &zoomLevel,
                             int targetGroupId) {
  if (m_loading)
    return;

  if (m_serverUrl.isEmpty()) {
    setError("Server not found yet. Waiting for discovery...");
    return;
  }

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl + "/api/fractals");
  QNetworkRequest request(url);
  if (!m_sessionToken.isEmpty()) {
    request.setRawHeader("Authorization",
                         ("Bearer " + m_sessionToken).toUtf8());
  }

  auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  QHttpPart titlePart;
  titlePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant("form-data; name=\"title\""));
  titlePart.setBody(title.toUtf8());
  multiPart->append(titlePart);

  QHttpPart authorPart;
  authorPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"author\""));
  authorPart.setBody(
      (author.isEmpty() ? QStringLiteral("Anonymous") : author).toUtf8());
  multiPart->append(authorPart);

  QHttpPart xpfPart;
  xpfPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                     QVariant("form-data; name=\"xpf\""));
  xpfPart.setBody(xpfData.toUtf8());
  multiPart->append(xpfPart);

  if (!formula.isEmpty()) {
    QHttpPart formulaPart;
    formulaPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant("form-data; name=\"formula\""));
    formulaPart.setBody(formula.toUtf8());
    multiPart->append(formulaPart);
  }

  QHttpPart iterPart;
  iterPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                     QVariant("form-data; name=\"iterations\""));
  iterPart.setBody(QByteArray::number(iterations));
  multiPart->append(iterPart);

  int uploadGroupId = (targetGroupId == -2) ? m_groupId : targetGroupId;
  if (uploadGroupId > 0) {
    QHttpPart groupPart;
    groupPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant("form-data; name=\"groupId\""));
    groupPart.setBody(QByteArray::number(uploadGroupId));
    multiPart->append(groupPart);
  }

  if (!zoomLevel.isEmpty()) {
    QHttpPart zoomPart;
    zoomPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"zoomLevel\""));
    zoomPart.setBody(zoomLevel.toUtf8());
    multiPart->append(zoomPart);
  }

  if (!thumbnailPath.isEmpty()) {
    QFile *thumbFile = new QFile(thumbnailPath);
    if (thumbFile->open(QIODevice::ReadOnly)) {
      QHttpPart thumbPart;
      thumbPart.setHeader(
          QNetworkRequest::ContentDispositionHeader,
          QVariant(
              "form-data; name=\"thumbnail\"; filename=\"thumbnail.png\""));
      thumbPart.setHeader(QNetworkRequest::ContentTypeHeader,
                          QVariant("image/png"));
      thumbPart.setBodyDevice(thumbFile);
      thumbFile->setParent(multiPart);
      multiPart->append(thumbPart);
    } else {
      delete thumbFile;
    }
  }

  QNetworkReply *reply = m_nam->post(request, multiPart);
  multiPart->setParent(reply);

  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onUploadFinished(reply); });
}

void CommunityClient::onUploadFinished(QNetworkReply *reply) {
  reply->deleteLater();
  setLoading(false);

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isObject() && doc.object().contains("error")) {
    setError(doc.object()["error"].toString());
    return;
  }

  if (reply->error() != QNetworkReply::NoError) {
    QString errStr = reply->errorString();
    if (!m_serverUrl.isEmpty()) {
      errStr.replace(m_serverUrl, "Server");
    }
    setError(QStringLiteral("Upload failed: ") + errStr);
    return;
  }

  QJsonObject obj = doc.object();

  int id = obj["id"].toInt();
  emit uploadComplete(id);
}


void CommunityClient::fetchGallery(int page, const QString &sort) {
  if (m_loading)
    return;

  if (m_serverUrl.isEmpty()) {
    setError("Server not found yet. Waiting for discovery...");
    return;
  }

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl + "/api/fractals");
  QUrlQuery query;
  query.addQueryItem("page", QString::number(page));
  query.addQueryItem("sort", sort);
  query.addQueryItem("limit", "20");
  if (m_groupId > 0) {
    query.addQueryItem("groupId", QString::number(m_groupId));
  }
  url.setQuery(query);

  QNetworkRequest request(url);
  if (!m_sessionToken.isEmpty()) {
    request.setRawHeader("Authorization",
                         ("Bearer " + m_sessionToken).toUtf8());
  }
  QNetworkReply *reply = m_nam->get(request);

  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onGalleryFinished(reply); });
}

void CommunityClient::onGalleryFinished(QNetworkReply *reply) {
  reply->deleteLater();
  setLoading(false);

  if (reply->error() != QNetworkReply::NoError) {
    setError(QStringLiteral("Gallery load failed: ") + reply->errorString());
    return;
  }

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) {
    setError("Invalid server response");
    return;
  }

  QJsonObject obj = doc.object();
  int totalPages = obj["totalPages"].toInt();
  int currentPage = obj["page"].toInt();

  QJsonArray arr = obj["items"].toArray();
  QVariantList items;
  items.reserve(arr.size());

  for (const QJsonValue &val : arr) {
    QJsonObject item = val.toObject();
    QVariantMap map;
    map["id"] = item["id"].toInt();
    map["title"] = item["title"].toString();
    map["author"] = item["author"].toString();
    map["formula"] = item["formula"].toString();
    map["iterations"] = item["iterations"].toInt();
    map["zoomLevel"] = item["zoom_level"].toString();
    map["downloads"] = item["downloads"].toInt();
    map["likes"] = item["likes"].toInt();
    map["createdAt"] = item["created_at"].toString();

    if (!item["thumbnailUrl"].isNull()) {
      map["thumbnailUrl"] = m_serverUrl + item["thumbnailUrl"].toString();
    } else {
      map["thumbnailUrl"] = QString();
    }

    items.append(map);
  }

  emit galleryLoaded(items, totalPages, currentPage);
}


void CommunityClient::downloadXpf(int fractalId) {
  if (m_loading)
    return;

  if (m_serverUrl.isEmpty()) {
    setError("Server not found yet. Waiting for discovery...");
    return;
  }

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl +
           QStringLiteral("/api/fractals/%1/xpf").arg(fractalId));
  QNetworkRequest request(url);
  QNetworkReply *reply = m_nam->get(request);

  connect(reply, &QNetworkReply::finished, this,
          [this, reply, fractalId]() { onXpfFinished(reply, fractalId); });
}

void CommunityClient::onXpfFinished(QNetworkReply *reply, int fractalId) {
  reply->deleteLater();
  setLoading(false);

  if (reply->error() != QNetworkReply::NoError) {
    setError(QStringLiteral("Download failed: ") + reply->errorString());
    return;
  }

  QString xpfData = QString::fromUtf8(reply->readAll());
  emit xpfDownloaded(fractalId, xpfData);
}


void CommunityClient::teacherLogin(const QString &email,
                                    const QString &password) {
  if (m_loading)
    return;

  if (m_serverUrl.isEmpty()) {
    setError("Server not found yet. Waiting for discovery...");
    return;
  }

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl + "/api/auth/teacher/login");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QJsonObject obj;
  obj["email"] = email;
  obj["password"] = password;

  QNetworkReply *reply = m_nam->post(request, QJsonDocument(obj).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onAuthFinished(reply); });
}

void CommunityClient::teacherSignup(const QString &email, const QString &password, const QString &displayName) {
  if (m_loading)
    return;

  if (m_serverUrl.isEmpty()) {
    setError("Server not found yet. Waiting for discovery...");
    return;
  }

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl + "/api/auth/teacher/signup");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  QJsonObject obj;
  obj["email"] = email;
  obj["password"] = password;
  obj["displayName"] = displayName;

  QNetworkReply *reply = m_nam->post(request, QJsonDocument(obj).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onAuthFinished(reply); });
}

void CommunityClient::joinGroup(const QString &inviteCode,
                                 const QString &displayName) {
  if (m_loading)
    return;

  if (m_serverUrl.isEmpty()) {
    setError("Server not found yet. Waiting for discovery...");
    return;
  }

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl + "/api/auth/student/join");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

  if (!m_sessionToken.isEmpty()) {
    request.setRawHeader("Authorization", ("Bearer " + m_sessionToken).toUtf8());
  }

  QJsonObject obj;
  obj["inviteCode"] = inviteCode;
  obj["displayName"] = displayName;

  QNetworkReply *reply = m_nam->post(request, QJsonDocument(obj).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onAuthFinished(reply); });
}

void CommunityClient::createRoom(const QString &roomName) {
  if (m_loading) return;
  if (m_serverUrl.isEmpty() || m_sessionToken.isEmpty()) return;

  setLoading(true);
  setError("");

  QUrl url(m_serverUrl + "/api/groups");
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setRawHeader("Authorization", ("Bearer " + m_sessionToken).toUtf8());

  QJsonObject obj;
  obj["name"] = roomName;

  QNetworkReply *reply = m_nam->post(request, QJsonDocument(obj).toJson());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onRoomCreated(reply); });
}

void CommunityClient::logout() {
  m_sessionToken.clear();
  m_userRole.clear();
  m_displayName.clear();
  m_groupId = -1;
  m_groupName.clear();
  m_userRooms.clear();

  QSettings settings("XaoS", "CommunityClient");
  settings.remove("sessionToken");
  settings.remove("userRole");
  settings.remove("displayName");

  emit authChanged();
  emit userRoomsChanged();
}

void CommunityClient::fetchUserRooms() {
  if (m_serverUrl.isEmpty() || m_sessionToken.isEmpty())
    return;

  QUrl url(m_serverUrl + "/api/user/rooms");
  QNetworkRequest request(url);
  request.setRawHeader("Authorization", ("Bearer " + m_sessionToken).toUtf8());
  QNetworkReply *reply = m_nam->get(request);
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onRoomsFinished(reply); });
}

void CommunityClient::leaveRoom(int roomId) {
  if (m_serverUrl.isEmpty() || m_sessionToken.isEmpty())
    return;

  QUrl url(m_serverUrl + QStringLiteral("/api/rooms/%1/leave").arg(roomId));
  QNetworkRequest request(url);
  request.setRawHeader("Authorization", ("Bearer " + m_sessionToken).toUtf8());
  QNetworkReply *reply = m_nam->post(request, QByteArray());
  connect(reply, &QNetworkReply::finished, this,
          [this, reply]() { onLeaveFinished(reply); });
}

void CommunityClient::selectRoom(int roomId, const QString &roomName) {
  if (m_groupId == roomId && m_groupName == roomName) return;
  m_groupId = roomId;
  m_groupName = roomName;
  emit authChanged();
}

void CommunityClient::likeFractal(int fractalId) {
  if (m_serverUrl.isEmpty())
    return;

  QUrl url(m_serverUrl +
           QStringLiteral("/api/fractals/%1/like").arg(fractalId));
  QNetworkRequest request(url);
  if (!m_sessionToken.isEmpty()) {
    request.setRawHeader("Authorization",
                         ("Bearer " + m_sessionToken).toUtf8());
  }
  QNetworkReply *reply = m_nam->post(request, QByteArray());
  connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
}

void CommunityClient::onAuthFinished(QNetworkReply *reply) {
  reply->deleteLater();
  setLoading(false);

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);

  if (reply->error() != QNetworkReply::NoError) {
    if (doc.isObject() && doc.object().contains("error")) {
      setError(doc.object()["error"].toString());
    } else {
      setError(QStringLiteral("Auth failed: ") + reply->errorString());
    }
    return;
  }

  if (!doc.isObject()) {
    setError("Invalid server response");
    return;
  }

  QJsonObject obj = doc.object();
  if (obj.contains("error")) {
    setError(obj["error"].toString());
    return;
  }

  m_sessionToken = obj["token"].toString();
  QJsonObject userObj = obj["user"].toObject();
  m_userRole = userObj["role"].toString();
  m_displayName = userObj["displayName"].toString();

  if (obj.contains("group")) {
    QJsonObject groupObj = obj["group"].toObject();
    m_groupId = groupObj["id"].toInt();
    m_groupName = groupObj["name"].toString();
  }

  QSettings settings("XaoS", "CommunityClient");
  settings.setValue("sessionToken", m_sessionToken);
  settings.setValue("userRole", m_userRole);
  settings.setValue("displayName", m_displayName);

  emit authChanged();
  emit loginSuccess();
  fetchUserRooms();
}

void CommunityClient::onRoomsFinished(QNetworkReply *reply) {
  reply->deleteLater();
  if (reply->error() != QNetworkReply::NoError) return;
  
  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) return;
  
  QJsonArray arr = doc.object()["rooms"].toArray();
  QVariantList rooms;
  for (const QJsonValue &val : arr) {
    QJsonObject item = val.toObject();
    QVariantMap map;
    map["id"] = item["id"].toInt();
    map["name"] = item["name"].toString();
    map["inviteCode"] = item["invite_code"].toString();
    map["createdBy"] = item["created_by"].toInt();
    rooms.append(map);
  }
  
  m_userRooms = rooms;
  emit userRoomsChanged();
}

void CommunityClient::onRoomCreated(QNetworkReply *reply) {
  setLoading(false);
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    QByteArray errData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(errData);
    if (doc.isObject() && doc.object().contains("error")) {
      setError(doc.object()["error"].toString());
    } else {
      setError("Failed to create room");
    }
    return;
  }

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isObject() && doc.object().contains("id")) {
    int id = doc.object()["id"].toInt();
    QString name = doc.object()["name"].toString();
    QString inviteCode = doc.object()["inviteCode"].toString();

    fetchUserRooms();
    selectRoom(id, name);
    emit roomCreated(id, name, inviteCode);
  }
}

void CommunityClient::onRoomMembersFinished(QNetworkReply *reply) {
  reply->deleteLater();
  if (reply->error() != QNetworkReply::NoError) return;
  
  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) return;
  
  QJsonArray arr = doc.object()["members"].toArray();
  QVariantList members;
  for (const QJsonValue &val : arr) {
    QJsonObject item = val.toObject();
    QVariantMap map;
    map["id"] = item["id"].toInt();
    map["displayName"] = item["display_name"].toString();
    map["role"] = item["role"].toString();
    map["joinedAt"] = item["joined_at"].toString();
    members.append(map);
  }
  
  emit roomMembersLoaded(members);
}

void CommunityClient::onLeaveFinished(QNetworkReply *reply) {
  reply->deleteLater();
  if (reply->error() == QNetworkReply::NoError) {
    m_groupId = -1;
    m_groupName = "";
    emit authChanged();
    fetchUserRooms();
  }
}
