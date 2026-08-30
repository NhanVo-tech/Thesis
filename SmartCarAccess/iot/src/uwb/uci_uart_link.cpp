#include "uwb/uci_uart_link.h"

namespace UwbUci {

UciUartLink::UciUartLink()
    : serial_(nullptr), callback_(), rxBuffer_(), ready_(false) {}

bool UciUartLink::begin(HardwareSerial& serial, int rxPin, int txPin, uint32_t baud) {
  serial_ = &serial;
  serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
  rxBuffer_.clear();
  ready_ = true;
  return true;
}

bool UciUartLink::sendPacket(Mt mt, uint8_t gid, uint8_t oid, const std::vector<uint8_t>& payload, uint8_t pbf) {
  if (!ready_ || !serial_) {
    return false;
  }

  std::vector<uint8_t> frame;
  frame.reserve(4 + payload.size());

  frame.push_back((static_cast<uint8_t>(mt) << 5) | ((pbf & 0x01) << 4) | (gid & 0x0F));
  frame.push_back(oid);

  if (mt == Mt::Data) {
    const uint16_t len = static_cast<uint16_t>(payload.size());
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
  } else {
    frame.push_back(0x00);
    frame.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
  }

  frame.insert(frame.end(), payload.begin(), payload.end());
  const size_t written = serial_->write(frame.data(), frame.size());
  serial_->flush();
  return written == frame.size();
}

void UciUartLink::setPacketCallback(PacketCallback cb) {
  callback_ = cb;
}

bool UciUartLink::isReady() const {
  return ready_ && serial_ != nullptr;
}

void UciUartLink::poll() {
  if (!isReady()) {
    return;
  }

  while (serial_->available() > 0) {
    const int b = serial_->read();
    if (b >= 0) {
      rxBuffer_.push_back(static_cast<uint8_t>(b));
    }
  }

  while (tryParseOnePacket()) {
  }
}

bool UciUartLink::tryParseOnePacket() {
  if (rxBuffer_.size() < 4) {
    return false;
  }

  const uint8_t h0 = rxBuffer_[0];
  const uint8_t h1 = rxBuffer_[1];
  const uint8_t h2 = rxBuffer_[2];
  const uint8_t h3 = rxBuffer_[3];

  const uint8_t mtRaw = static_cast<uint8_t>((h0 & 0xE0) >> 5);
  if (mtRaw > static_cast<uint8_t>(Mt::Notification)) {
    rxBuffer_.erase(rxBuffer_.begin());
    return true;
  }

  const Mt mt = static_cast<Mt>(mtRaw);
  const uint16_t payloadLen = (mt == Mt::Data)
                                  ? static_cast<uint16_t>(h2 | (static_cast<uint16_t>(h3) << 8))
                                  : static_cast<uint16_t>(h3);
  const size_t fullLen = static_cast<size_t>(4 + payloadLen);
  if (rxBuffer_.size() < fullLen) {
    return false;
  }

  UciPacket packet;
  packet.mt = mt;
  packet.gid = static_cast<uint8_t>(h0 & 0x0F);
  packet.oid = h1;
  packet.pbf = static_cast<uint8_t>((h0 & 0x10) >> 4);
  packet.payload.assign(rxBuffer_.begin() + 4, rxBuffer_.begin() + fullLen);

  rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + fullLen);
  if (callback_) {
    callback_(packet);
  }
  return true;
}

}  // namespace UwbUci
