import CoreBluetooth
import Observation

// Talks to the relay (the ESP32) over Bluetooth. Screens only read
// `status` and call send(); nothing else needs to open this file.
// The message format is in PROTOCOL.md at the top of the repo.
@Observable
class Relay: NSObject {
    // What to show on screen, e.g. "🔗 relay connected".
    var status = "🔍 looking for the relay"

    // From PROTOCOL.md. The relay uses the same ones.
    private let serviceID = CBUUID(string: "6E1D0001-3C5A-4B8E-9F21-7A4D2C8B5E10")
    private let controlsID = CBUUID(string: "6E1D0002-3C5A-4B8E-9F21-7A4D2C8B5E10")

    private var central: CBCentralManager!
    private var relay: CBPeripheral?
    private var controls: CBCharacteristic?

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main)
    }

    // Sends one controls message. Does nothing until the relay is connected.
    func send(_ flight: Flight) {
        guard let relay, let controls, relay.canSendWriteWithoutResponse else { return }
        let message: [UInt8] = [
            1,  // version
            byte(flight.forward),
            byte(flight.right),
            byte(flight.turn),
            byte(flight.up),
        ]
        relay.writeValue(Data(message), for: controls, type: .withoutResponse)
    }

    private func lookForRelay() {
        relay = nil
        controls = nil
        status = "🔍 looking for the relay"
        central.scanForPeripherals(withServices: [serviceID])
    }
}

// -100 to 100, as one signed byte.
private func byte(_ percent: Double) -> UInt8 {
    let whole = Int(min(max(percent, -100), 100).rounded())
    return UInt8(bitPattern: Int8(whole))
}

extension Relay: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            lookForRelay()
        case .poweredOff:
            status = "📴 Bluetooth is off"
        case .unauthorized:
            status = "🚫 allow Bluetooth for Fledge in Settings"
        default:
            status = "❓ no Bluetooth"
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        central.stopScan()
        relay = peripheral
        status = "🔌 connecting"
        central.connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.delegate = self
        peripheral.discoverServices([serviceID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral,
                        error: Error?) {
        lookForRelay()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?) {
        lookForRelay()
    }
}

extension Relay: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == serviceID }) else { return }
        peripheral.discoverCharacteristics([controlsID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        controls = service.characteristics?.first(where: { $0.uuid == controlsID })
        if controls != nil {
            status = "🔗 relay connected"
        }
    }
}
