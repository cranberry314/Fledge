// Pretends to be the relay, so the phone app's Bluetooth can be tested
// on the Mac, without an ESP32. It advertises the relay's service from
// PROTOCOL.md and prints each controls message the phone sends, whenever
// the values change.
//
//   swift firmware/StandIn/StandIn.swift
//
// The first time, macOS asks whether the terminal app may use Bluetooth:
// allow it. Stop with Ctrl-C.

import CoreBluetooth
import Foundation

// From PROTOCOL.md.
let serviceID = CBUUID(string: "6E1D0001-3C5A-4B8E-9F21-7A4D2C8B5E10")
let controlsID = CBUUID(string: "6E1D0002-3C5A-4B8E-9F21-7A4D2C8B5E10")

class StandIn: NSObject, CBPeripheralManagerDelegate {
    var manager: CBPeripheralManager!
    var lastMessage: [UInt8] = []
    var lastTime = Date()
    var stopped = true  // true until the first message, and after 500 ms of silence

    override init() {
        super.init()
        manager = CBPeripheralManager(delegate: self, queue: .main)
        // Check for silence 10 times a second, as the relay will.
        Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
            self.checkSilence()
        }
    }

    func peripheralManagerDidUpdateState(_ peripheral: CBPeripheralManager) {
        guard peripheral.state == .poweredOn else {
            say("Bluetooth isn't ready (state \(peripheral.state.rawValue)); is it on, and allowed?")
            return
        }
        let controls = CBMutableCharacteristic(type: controlsID, properties: [.writeWithoutResponse],
                                               value: nil, permissions: [.writeable])
        let service = CBMutableService(type: serviceID, primary: true)
        service.characteristics = [controls]
        peripheral.add(service)
    }

    func peripheralManager(_ peripheral: CBPeripheralManager, didAdd service: CBService, error: Error?) {
        if let error {
            say("Couldn't add the service: \(error.localizedDescription)")
            return
        }
        peripheral.startAdvertising([
            CBAdvertisementDataLocalNameKey: "Fledge Relay",
            CBAdvertisementDataServiceUUIDsKey: [serviceID],
        ])
    }

    func peripheralManagerDidStartAdvertising(_ peripheral: CBPeripheralManager, error: Error?) {
        if let error {
            say("Couldn't advertise: \(error.localizedDescription)")
            return
        }
        say("Advertising as Fledge Relay. Open Fledge on the phone.")
    }

    func peripheralManager(_ peripheral: CBPeripheralManager, didReceiveWrite requests: [CBATTRequest]) {
        for request in requests {
            let message = [UInt8](request.value ?? Data())
            lastTime = Date()
            if stopped {
                stopped = false
                say("📨 messages arriving")
            }
            if message == lastMessage {
                continue
            }
            lastMessage = message
            say(describe(message))
        }
    }

    func checkSilence() {
        if !stopped && Date().timeIntervalSince(lastTime) > 0.5 {
            stopped = true
            lastMessage = []
            say("⚠️ no message for 500 ms: the relay would set everything to 0")
        }
    }
}

func describe(_ message: [UInt8]) -> String {
    guard message.count == 5, message[0] == 1 else {
        let hex = message.map { String(format: "%02X", $0) }.joined(separator: " ")
        return "❓ unexpected message: \(hex)"
    }
    func value(_ i: Int) -> String {
        String(format: "%4d", Int(Int8(bitPattern: message[i])))
    }
    return "forward \(value(1))   right \(value(2))   turn \(value(3))   up \(value(4))"
}

func say(_ text: String) {
    let time = Date().formatted(date: .omitted, time: .standard)
    print("\(time)  \(text)")
}

let standIn = StandIn()
RunLoop.main.run()
