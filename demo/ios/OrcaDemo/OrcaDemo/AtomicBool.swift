import Foundation

class AtomicBool {
    private var value: Bool
    private let lock = NSLock()

    init(_ value: Bool = false) {
        self.value = value
    }

    func set(_ newValue: Bool) {
        lock.lock()
        value = newValue
        lock.unlock()
    }

    func get() -> Bool {
        lock.lock()
        defer { lock.unlock() }
        return value
    }
}
