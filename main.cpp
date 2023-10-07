#include <QCoreApplication>

#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>

#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergyservice.h>

const QString deviceName = "BerryMed";

const QBluetoothUuid serviceAddress = QUuid("49535343-fe7d-4ae5-8fa9-9fafd205e455");
bool foundService = false;

const QBluetoothUuid dataReceive = QBluetoothUuid(QUuid("49535343-1e4d-4bd9-ba61-23c647249616"));
const QBluetoothUuid receieveDescriptor = QBluetoothUuid(QUuid("00002902-0000-1000-8000-00805f9b34fb"));

const QByteArray ENABLED = QByteArray::fromHex("0100");
const QByteArray DISABLED = QByteArray::fromHex("0000");

QLowEnergyController *controller;
QLowEnergyService *m_service;

void foundDevice(const QBluetoothDeviceInfo &device);
void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
void scanFinished();

void connected();
void disconnected();
void connectionError(QLowEnergyController::Error error);

void serviceDiscovered(const QBluetoothUuid &newService);
void serviceScanDone();

void serviceStateChanged(QLowEnergyService::ServiceState newState);
void serviceError(QLowEnergyService::ServiceError error);

int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);

    auto deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent();
    deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(10000);

    QObject::connect(
        deviceDiscoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        foundDevice
    );
    QObject::connect(
        deviceDiscoveryAgent,
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                &QBluetoothDeviceDiscoveryAgent::errorOccurred,
        #else
                static_cast<
                    void (QBluetoothDeviceDiscoveryAgent::*)(QBluetoothDeviceDiscoveryAgent::Error)
                    >(&QBluetoothDeviceDiscoveryAgent::error),
        #endif
        scanError
    );

    QObject::connect(
        deviceDiscoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::finished,
        scanFinished
    );
    QObject::connect(
        deviceDiscoveryAgent,
        &QBluetoothDeviceDiscoveryAgent::canceled,
        scanFinished
    );

    deviceDiscoveryAgent->start();

    return a.exec();

}

void foundDevice(const QBluetoothDeviceInfo &device)
{
    qDebug() << "Found device: " << device.name();
    if (device.name().contains(deviceName)) {
        qDebug() << "Device discovered!\n";

        controller = QLowEnergyController::createCentral(device);

        QObject::connect(
            controller,
            &QLowEnergyController::serviceDiscovered,
            serviceDiscovered
        );
        QObject::connect(
            controller,
            &QLowEnergyController::discoveryFinished,
            serviceScanDone
        );

        QObject::connect(
            controller,
            #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                &QLowEnergyController::errorOccurred,
            #else
                static_cast<
                    void (QLowEnergyController::*)(QLowEnergyController::Error)
                >(&QLowEnergyController::error),
            #endif
            connectionError
        );
        QObject::connect(
            controller,
            &QLowEnergyController::connected,
            connected
        );
        QObject::connect(
            controller,
            &QLowEnergyController::disconnected,
            disconnected
        );

        qDebug() << "Connecting";
        controller->connectToDevice();
    }
}

void scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{ qDebug() << "Scan error: " << error; }

void scanFinished()
{ qDebug() << "Scan finished"; }

void connected()
{
    qDebug() << "Connected";
    controller->discoverServices();
}

void connectionError(QLowEnergyController::Error error)
{ qDebug() << "Connection error: " << error; }

void disconnected()
{ qDebug() << "Disconnected"; }

void serviceDiscovered(const QBluetoothUuid &newService)
{
    qDebug() << "Found service: " << newService;
    if (newService == serviceAddress)
        foundService = true;
}

void serviceScanDone()
{
    qDebug() << "Services scan done";
    if (foundService){
        qDebug() << "Service Discovered!";

        m_service = controller->createServiceObject(serviceAddress);

        QObject::connect(
            m_service,
            #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                &QLowEnergyService::errorOccurred,
            #else
                static_cast<
                    void (QLowEnergyService::*)(QLowEnergyService::ServiceError)
                >(&QLowEnergyService::error),
            #endif
            serviceError
        );
        QObject::connect(
            m_service,
            &QLowEnergyService::stateChanged,
            serviceStateChanged
        );
        QObject::connect(
            m_service,
            &QLowEnergyService::descriptorWritten,
                [](const QLowEnergyDescriptor &info, const QByteArray &value){
                    if (value == ENABLED)
                        qDebug() << "Notification Set!!!";
                });
        QObject::connect(m_service, &QLowEnergyService::characteristicChanged,
                         [](const QLowEnergyCharacteristic &c, const QByteArray &value) {
            qDebug() << value;
        });

        if (m_service->state() == QLowEnergyService::DiscoveryRequired)
            m_service->discoverDetails();
    }
}

void serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    qDebug() << "Service state changed: " << newState;
    if (newState == QLowEnergyService::ServiceDiscovered) {
        auto c = m_service->characteristic(dataReceive);
        if (c.isValid()) {
            m_service->writeDescriptor(c.descriptor(receieveDescriptor), ENABLED);
        }
    }
}

void serviceError(QLowEnergyService::ServiceError error)
{ qDebug() << "Service error: " << error; }
