#include "modules/network/network_server.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDataStream>
#include <QTcpSocket>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>

#include <iostream>
#include <nlohmann/json.hpp>

namespace SnowOwl::Server::Modules::Network {

NetworkServer::NetworkServer(quint16 port, QObject* parent)
    : QObject(parent)
    , tcpServer_(new QTcpServer(this))
    , keepaliveTimer_(new QTimer(this))
    , port_(port)
{
    connect(tcpServer_, &QTcpServer::newConnection, this, &NetworkServer::onNewConnection);
    connect(keepaliveTimer_, &QTimer::timeout, this, &NetworkServer::onSendKeepalive);
    
    keepaliveTimer_->setInterval(30000);
    keepaliveTimer_->start();
}

NetworkServer::~NetworkServer()
{
    stop();
}

bool NetworkServer::listen(quint16 port)
{
    if (port != 0) {
        port_ = port;
    }
    return tcpServer_->listen(QHostAddress::Any, port_);
}

void NetworkServer::stop()
{
    for (auto* client : clients_) {
        if (client && client->socket) {
            client->socket->disconnectFromHost();
        }
    }
    
    clients_.clear();
    tcpServer_->close();
}

void NetworkServer::broadcastFrame(const cv::Mat& frame)
{
    emit frameReceived(frame);
}

void NetworkServer::broadcastEvents(const std::vector<Detection::DetectionResult>& events)
{
    QJsonArray eventsArray;
    for (const auto& event : events) {
        QJsonObject eventObj;
        eventObj["type"] = QString::fromStdString(Detection::detectionTypeToString(event.type));
        eventObj["confidence"] = event.confidence;
        eventObj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        
        QJsonObject bbox;
        bbox["x"] = event.boundingBox.x;
        bbox["y"] = event.boundingBox.y;
        bbox["width"] = event.boundingBox.width;
        bbox["height"] = event.boundingBox.height;
        eventObj["bounding_box"] = bbox;
        
        eventObj["description"] = QString::fromStdString(event.description);
        eventsArray.append(eventObj);
    }
    
    QJsonObject message;
    message["type"] = "detection_events";
    message["events"] = eventsArray;
    message["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    sendToAllClients({{"type", "detection_events"}, {"data", QString::fromUtf8(jsonData)}});
}

void NetworkServer::onNewConnection()
{
    while (tcpServer_->hasPendingConnections()) {
        QTcpSocket* socket = tcpServer_->nextPendingConnection();
        if (!socket) continue;
        
        auto* client = new Client{socket, new QTimer(this), false};
        client->keepaliveTimer->setSingleShot(true);
        client->keepaliveTimer->setInterval(60000);
        connect(client->keepaliveTimer, &QTimer::timeout, this, [this, client]() {
            if (client && client->socket) {
                client->socket->disconnectFromHost();
            }
        });
        
        connect(socket, &QTcpSocket::disconnected, this, [this, client]() {
            onClientDisconnected();
            cleanupClient(client);
        });
        connect(socket, &QTcpSocket::readyRead, this, [this, client]() {
            onReadyRead();
        });
        connect(socket, &QTcpSocket::errorOccurred, this, [this, socket]() {
            onError(socket->error());
        });
        
        clients_.append(client);
        client->keepaliveTimer->start();
        
        emit clientConnected();
    }
}

void NetworkServer::onClientDisconnected()
{
    emit clientDisconnected();
}

void NetworkServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    

    Client* client = nullptr;
    for (auto* c : clients_) {
        if (c && c->socket == socket) {
            client = c;
            break;
        }
    }
    
    if (!client) return;
    
    client->keepaliveTimer->start();
    
    while (socket->bytesAvailable() > 0) {
        QByteArray data = socket->readAll();
        handleClientMessage(client, data);
    }
}

void NetworkServer::onSendKeepalive()
{
    sendToAllClients({{"type", "keepalive"}, {"timestamp", QDateTime::currentMSecsSinceEpoch()}});
}

void NetworkServer::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred(tr("Socket error occurred"));
}

void NetworkServer::setupServer()
{

}

void NetworkServer::handleClientMessage(Client* client, const QByteArray& message)
{
    Q_UNUSED(client)
    Q_UNUSED(message)
}

void NetworkServer::sendToClient(Client* client, const QVariantMap& data)
{
    if (!client || !client->socket) return;
    
    QJsonDocument doc(QJsonObject::fromVariantMap(data));
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_0);
    out << quint32(0);
    out << jsonData;
    
    out.device()->seek(0);
    out << quint32(block.size() - sizeof(quint32));
    
    client->socket->write(block);
}

void NetworkServer::sendToAllClients(const QVariantMap& data)
{
    for (auto it = clients_.begin(); it != clients_.end();) {
        auto* client = *it;
        if (!client || !client->socket) {
            it = clients_.erase(it);
            continue;
        }
        
        if (client->socket->state() != QAbstractSocket::ConnectedState) {
            cleanupClient(client);
            it = clients_.erase(it);
            continue;
        }
        
        sendToClient(client, data);
        ++it;
    }
}

void NetworkServer::cleanupClient(Client* client) {
    if (!client) return;
    
    if (client->keepaliveTimer) {
        client->keepaliveTimer->stop();
        client->keepaliveTimer->deleteLater();
    }
    
    if (client->socket) {
        client->socket->deleteLater();
    }
    
    delete client;
}

}