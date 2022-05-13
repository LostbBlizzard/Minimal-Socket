/**
 * Author:    Andrea Casalino
 * Created:   01.28.2020
 *
 * report any bug to andrecasa91@gmail.com.
 **/

#pragma once

#include <MinimalSocket/Error.h>
#include <MinimalSocket/core/Definitions.h>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace MinimalSocket {
#ifdef _WIN32
using WSAVersion = std::array<std::uint16_t, 2>;

class WSAManager {
public:
  static void setWsaVersion(const WSAVersion &version);
  static WSAVersion getWsaVersion();

private:
  static std::mutex wsa_version_mtx;
  static WSAVersion wsa_version;
};
#endif

class SocketIdWrapper;

class Socket {
public:
  virtual ~Socket();

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  bool empty() const;

  int accessSocketID() const;

protected:
  Socket();

  static void transfer(Socket &receiver, Socket &giver);

  const SocketIdWrapper &getIDWrapper() const;
  SocketIdWrapper &getIDWrapper();
  void resetIDWrapper();

private:
  std::unique_ptr<SocketIdWrapper> socket_id_wrapper;
};

bool operator==(std::nullptr_t, const Socket &subject);
bool operator==(const Socket &subject, std::nullptr_t);

class Openable : public virtual Socket {
public:
  bool wasOpened() const { return opened; }
  std::unique_ptr<Error> open(const Timeout &timeout = NULL_TIMEOUT);

protected:
  Openable() = default;

  static void transfer(Openable &receiver,
                       Openable &giver); // Socket::transfer(...) is also called

  virtual void open_() = 0;

private:
  std::mutex open_procedure_mtx;
  std::atomic_bool opened = false;
};
} // namespace MinimalSocket
