#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QVariantMap>
#include <memory>
#include <vector>

#include <opencv2/core/mat.hpp>

#include "detection/detection_types.hpp"

namespace SnowOwl::Server::Modules::Network {

class NetworkServer : public QObject {
    Q_OBJECT

public:
    explicit NetworkServer(quint16 port = 8000, QObject* parent = nullptr);
    ~NetworkServer() override;

    bool listen(quint16 port = 0);
    void stop();
    
    bool startNetworkSystem() { return listen(0); }
    void stopNetworkSystem() { stop(); }
    void broadcastFrame(const cv::Mat& frame);
    void broadcastEvents(const std::vector<Detection::DetectionResult>& events);

signals:
    void clientConnected();
    void clientDisconnected();
    void errorOccurred(const QString& error);
    void frameReceived(const cv::Mat& frame);
    void detectionReceived(const std::vector<Detection::DetectionResult>& detections);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
    void onSendKeepalive();
    void onError(QAbstractSocket::SocketError socketError);

private:
    struct Client {
        QTcpSocket* socket;
        QTimer* keepaliveTimer;
        bool authenticated;
    };

    void setupServer();
    void handleClientMessage(Client* client, const QByteArray& message);
    void sendToClient(Client* client, const QVariantMap& data);
    void sendToAllClients(const QVariantMap& data);
    void cleanupClient(Client* client);

    QTcpServer* tcpServer_;
    QList<Client*> clients_;
    QTimer* keepaliveTimer_;
    quint16 port_;
};

}