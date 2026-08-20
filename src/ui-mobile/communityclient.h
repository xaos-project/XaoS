#ifndef COMMUNITYCLIENT_H
#define COMMUNITYCLIENT_H

#include <QObject>
#include <QString>
#include <QVariantList>

class QNetworkAccessManager;
class QNetworkReply;
class QUdpSocket;

/**
 * @brief HTTP client for the XaoS Community Sharing Server.
 *
 * Provides methods to upload, browse, and download fractal positions
 * via the community REST API. All network operations are asynchronous;
 * results are delivered via signals.
 *
 * On construction, automatically listens for the server's UDP discovery
 * beacon on port 3001 to find the server without hardcoded IPs.
 *
 * Exposed to QML as a context property ("community").
 */
class CommunityClient : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
  Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
  Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
  Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY authChanged)
  Q_PROPERTY(QString currentUserRole READ currentUserRole NOTIFY authChanged)
  Q_PROPERTY(int currentGroupId READ currentGroupId NOTIFY authChanged)
  Q_PROPERTY(QString currentGroupName READ currentGroupName NOTIFY authChanged)
  Q_PROPERTY(bool serverDiscovered READ isServerDiscovered NOTIFY serverDiscovered)
  Q_PROPERTY(QString serverStatus READ serverStatus NOTIFY serverStatusChanged)
  Q_PROPERTY(QString currentDisplayName READ currentDisplayName NOTIFY authChanged)
  Q_PROPERTY(QVariantList userRooms READ userRooms NOTIFY userRoomsChanged)

public:
  explicit CommunityClient(QObject *parent = nullptr);

  bool isLoading() const { return m_loading; }
  QString errorMessage() const { return m_error; }
  QString serverUrl() const { return m_serverUrl; }
  void setServerUrl(const QString &url);

  bool isLoggedIn() const { return !m_sessionToken.isEmpty(); }
  QString currentUserRole() const { return m_userRole; }
  int currentGroupId() const { return m_groupId; }
  QString currentGroupName() const { return m_groupName; }
  bool isServerDiscovered() const { return m_serverFound; }
  QString serverStatus() const { return m_serverStatus; }
  QString currentDisplayName() const { return m_displayName; }
  QVariantList userRooms() const { return m_userRooms; }

public slots:
  Q_INVOKABLE void clearError() { setError(""); }

  Q_INVOKABLE void fetchRoomMembers(int roomId);

  /**
   * Upload a fractal to the community server.
   */
  Q_INVOKABLE void upload(const QString &title, const QString &author,
                           const QString &xpfData,
                           const QString &thumbnailPath,
                           const QString &formula, int iterations,
                           const QString &zoomLevel,
                           int targetGroupId = -2);

  /**
   * Fetch a page of community fractals.
   */
  Q_INVOKABLE void fetchGallery(int page = 1, const QString &sort = "recent");

  Q_INVOKABLE void teacherLogin(const QString &email, const QString &password);
  Q_INVOKABLE void teacherSignup(const QString &email, const QString &password, const QString &displayName);
  Q_INVOKABLE void joinGroup(const QString &inviteCode, const QString &displayName);
  Q_INVOKABLE void likeFractal(int fractalId);
  Q_INVOKABLE void logout();
  Q_INVOKABLE void fetchUserRooms();
  Q_INVOKABLE void leaveRoom(int roomId);
  Q_INVOKABLE void selectRoom(int roomId, const QString &roomName);
  Q_INVOKABLE void createRoom(const QString &roomName);
  Q_INVOKABLE void resetToDefaultUrl();
  /**
   * Download the .xpf data for a specific fractal.
   */
  Q_INVOKABLE void downloadXpf(int fractalId);

  /**
   * Ask the configured server whether it is actually there, via /api/health.
   * Updates serverStatus; safe to call at any time.
   */
  Q_INVOKABLE void probeServer();

signals:
  void loadingChanged();
  void errorChanged();
  void serverUrlChanged();
  void authChanged();
  void loginSuccess();
  void serverDiscovered();
  void serverStatusChanged();
  void userRoomsChanged();
  void roomCreated(int id, const QString &name, const QString &inviteCode);
  void roomMembersLoaded(QVariantList members);

  void uploadComplete(int id);

  void galleryLoaded(const QVariantList &items, int totalPages, int currentPage);

  void xpfDownloaded(int fractalId, const QString &xpfData);

  void networkError(const QString &message);

private slots:
  void onUploadFinished(QNetworkReply *reply);
  void onGalleryFinished(QNetworkReply *reply);
  void onXpfFinished(QNetworkReply *reply, int fractalId);
  void onAuthFinished(QNetworkReply *reply);
  void onRoomsFinished(QNetworkReply *reply);
  void onLeaveFinished(QNetworkReply *reply);
  void onRoomCreated(QNetworkReply *reply);
  void onRoomMembersFinished(QNetworkReply *reply);
  void onDiscoveryDatagram();

private:
  void setLoading(bool loading);
  void setError(const QString &error);
  void startDiscovery();
  void setServerStatus(const QString &status);
  void noteReplyOutcome(QNetworkReply *reply);

  QNetworkAccessManager *m_nam;
  QUdpSocket *m_discoverySocket = nullptr;
  bool m_loading = false;
  bool m_serverFound = false;
  QString m_serverStatus = QStringLiteral("unknown");
  QNetworkReply *m_probeReply = nullptr;
  QString m_error;
  QString m_serverUrl;

  QString m_sessionToken;
  QString m_userRole;
  QString m_displayName;
  int m_groupId = -1;
  QString m_groupName;
  QVariantList m_userRooms;
};

#endif
