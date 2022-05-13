#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <omp.h>
#include <thread>

#include <MinimalSocket/udp/UdpSocket.h>

#include "Parallel.h"
#include "PortFactory.h"

using namespace MinimalSocket;
using namespace MinimalSocket::udp;
using namespace MinimalSocket::test;

namespace {
static const std::string request = "Hello";
static const std::string response = "Welcome";

bool are_same(const Address &a, const Address &b, const AddressFamily &family) {
  return (family == AddressFamily::IP_V4) ? (a == b)
                                          : (a.getPort() == b.getPort());
}
} // namespace

TEST_CASE("Exchange messages between UdpBinded and UdpBinded", "[udp]") {
  const auto family = GENERATE(IP_V4, IP_V6);
  const std::size_t cycles = 5;

  const auto requester_port = PortFactory::makePort();
  const Address requester_address = Address(requester_port, family);
  UdpBinded requester(requester_port, family);
  REQUIRE(requester.open());

  const auto responder_port = PortFactory::makePort();
  const Address responder_address = Address(responder_port, family);
  UdpBinded responder(responder_port, family);
  REQUIRE(responder.open());

  parallel(
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
          CHECK(requester.sendTo(request, responder_address));
#pragma omp barrier
#pragma omp barrier
          auto received_response = requester.receive(response.size());
          REQUIRE(received_response);
          CHECK(received_response->received_message == response);
          CHECK(are_same(received_response->sender, responder_address, family));
        }
      },
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
#pragma omp barrier
          auto received_request = responder.receive(request.size());
          REQUIRE(received_request);
          CHECK(received_request->received_message == request);
          CHECK(are_same(received_request->sender, requester_address, family));
          responder.sendTo(response, requester_address);
#pragma omp barrier
        }
      });

  SECTION("receive with timeout") {
    const auto timeout = Timeout{500};

    SECTION("expect fail within timeout") {
      auto received_request = responder.receive(request.size(), timeout);
      CHECK_FALSE(received_request);
    }

    SECTION("expect success within timeout") {
      const auto wait = Timeout{250};
      parallel(
          [&]() {
#pragma omp barrier
            std::this_thread::sleep_for(wait);
            requester.sendTo(request, responder_address);
          },
          [&]() {
#pragma omp barrier
            auto received_request = responder.receive(request.size(), timeout);
            REQUIRE(received_request);
            CHECK(received_request->received_message == request);
            CHECK(
                are_same(received_request->sender, requester_address, family));
          });
    }
  }
}

TEST_CASE("Exchange messages between UdpConnected and UdpConnected", "[udp]") {
  const auto family = GENERATE(IP_V4, IP_V6);
  const std::size_t cycles = 5;

  const auto requester_port = PortFactory::makePort();
  const Address requester_address = Address(requester_port, family);

  const auto responder_port = PortFactory::makePort();
  const Address responder_address = Address(responder_port, family);

  UdpConnected requester(responder_address, requester_port);
  REQUIRE(requester.open());
  UdpConnected responder(requester_address, responder_port);
  REQUIRE(responder.open());

  parallel(
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
          CHECK(requester.send(request));
#pragma omp barrier
#pragma omp barrier
          auto received_response = requester.receive(response.size());
          CHECK(received_response == response);
        }
      },
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
#pragma omp barrier
          auto received_request = responder.receive(request.size());
          CHECK(received_request == request);
          responder.send(response);
#pragma omp barrier
        }
      });

  SECTION("receive with timeout") {
    const auto timeout = Timeout{500};

    SECTION("expect fail within timeout") {
      auto received_request = responder.receive(request.size(), timeout);
      CHECK(received_request.empty());
    }

    SECTION("expect success within timeout") {
      const auto wait = Timeout{250};
      parallel(
          [&]() {
#pragma omp barrier
            std::this_thread::sleep_for(wait);
            requester.send(request);
          },
          [&]() {
#pragma omp barrier
            auto received_request = responder.receive(request.size(), timeout);
            CHECK(received_request == request);
          });
    }
  }
}

TEST_CASE(
    "Receive from thirdy peer expected to fail after udp socket connected",
    "[udp]") {
  const auto family = GENERATE(IP_V4, IP_V6);

  const auto requester_port = PortFactory::makePort();
  const Address requester_address = Address(requester_port, family);

  const auto responder_port = PortFactory::makePort();
  const Address responder_address = Address(responder_port, family);

  UdpConnected requester(responder_address, requester_port);
  REQUIRE(requester.open());
  UdpConnected responder(requester_address, responder_port);
  REQUIRE(responder.open());

  auto exchange_messages_before = GENERATE(true, false);
  if (exchange_messages_before) {
    parallel(
        [&]() {
          CHECK(requester.send(request));
#pragma omp barrier
#pragma omp barrier
          auto received_response = requester.receive(response.size());
          CHECK(received_response == response);
        },
        [&]() {
#pragma omp barrier
          auto received_request = responder.receive(request.size());
          CHECK(received_request == request);
          responder.send(response);
#pragma omp barrier
        });
  }

  UdpBinded second_requester(PortFactory::makePort(), family);
  REQUIRE(second_requester.open());
  const auto timeout = Timeout{500};
  const auto wait = Timeout{250};
  parallel(
      [&]() {
#pragma omp barrier
        std::this_thread::sleep_for(wait);
        second_requester.sendTo(request, Address(responder_port, family));
      },
      [&]() {
#pragma omp barrier
        auto received_request = responder.receive(request.size(), timeout);
        CHECK(received_request.empty());
      });
}

TEST_CASE("Metamorphosis of udp connections", "[udp]") {
  const auto family = GENERATE(IP_V4, IP_V6);
  const std::size_t cycles = 5;

  const auto requester_port = PortFactory::makePort();
  const Address requester_address = Address(requester_port, family);
  const auto responder_port = PortFactory::makePort();
  const Address responder_address = Address(responder_port, family);

  UdpBinded responder(responder_port, family);
  REQUIRE(responder.open());

  std::unique_ptr<UdpBinded> requester_only_bind =
      std::make_unique<UdpBinded>(requester_port, family);
  REQUIRE(requester_only_bind->open());

  // connect requester to responder
  auto deduce_sender = GENERATE(true, false);
  std::unique_ptr<UdpConnected> requester_connected;
  if (deduce_sender) {
    parallel(
        [&]() {
          responder.sendTo("1", requester_address);
#pragma omp barrier
        },
        [&]() {
#pragma omp barrier
          auto socket_connected = requester_only_bind->connect();
          CHECK(are_same(socket_connected.getRemoteAddress(), responder_address,
                         family));
          requester_connected =
              std::make_unique<UdpConnected>(std::move(socket_connected));
        });
  } else {
    requester_connected = std::make_unique<UdpConnected>(
        requester_only_bind->connect(responder_address));
  }
  REQUIRE(requester_connected->wasOpened());

  // try message exchange
  parallel(
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
          CHECK(requester_connected->send(request));
#pragma omp barrier
#pragma omp barrier
          auto received_response =
              requester_connected->receive(response.size());
          CHECK(received_response == response);
        }
      },
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
#pragma omp barrier
          auto received_request = responder.receive(request.size());
          REQUIRE(received_request);
          CHECK(received_request->received_message == request);
          responder.sendTo(response, requester_address);
#pragma omp barrier
        }
      });

  // try to disconnect requester
  requester_only_bind =
      std::make_unique<UdpBinded>(requester_connected->disconnect());
  REQUIRE(requester_only_bind->wasOpened());

  // try message exchange
  parallel(
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
          CHECK(requester_only_bind->sendTo(request, responder_address));
#pragma omp barrier
#pragma omp barrier
          auto received_response =
              requester_only_bind->receive(response.size());
          REQUIRE(received_response);
          CHECK(received_response->received_message == response);
        }
      },
      [&]() {
        for (std::size_t c = 0; c < cycles; ++c) {
#pragma omp barrier
          auto received_request = responder.receive(request.size());
          REQUIRE(received_request);
          CHECK(received_request->received_message == request);
          responder.sendTo(response, requester_address);
#pragma omp barrier
        }
      });
}

TEST_CASE("Open connection with timeout", "[udp]") {
  const auto family = GENERATE(IP_V4, IP_V6);

  const auto requester_port = PortFactory::makePort();
  const Address requester_address = Address(requester_port, family);
  UdpBinded requester(requester_port, family);
  REQUIRE(requester.open());

  const auto responder_port = PortFactory::makePort();
  const Address responder_address = Address(responder_port, family);
  UdpBinded responder(responder_port, family);
  REQUIRE(responder.open());

  const auto timeout = Timeout{500};

  SECTION("expect fail within timeout") {
    CHECK_FALSE(requester.connect(timeout));
  }

  SECTION("expect success within timeout") {
    const auto wait = Timeout{250};
    parallel(
        [&]() {
#pragma omp barrier
          std::this_thread::sleep_for(wait);
          responder.sendTo("1", requester_address);
        },
        [&]() {
#pragma omp barrier
          auto connected_result = requester.connect(timeout);
          REQUIRE(connected_result);
          CHECK(are_same(connected_result->getRemoteAddress(),
                         responder_address, family));
        });
  }
}

TEST_CASE("Reserve random port for udp connection", "[udp]") {
  const auto family = GENERATE(IP_V4, IP_V6);

  auto requester_port = ANY_PORT;
  UdpBinded requester(requester_port, family);
  REQUIRE(requester.open());
  requester_port = requester.getPortToBind();
  const Address requester_address = Address(requester_port, family);

  auto responder_port = GENERATE(PortFactory::makePort(), ANY_PORT);
  UdpBinded responder(responder_port, family);
  REQUIRE(responder.open());
  responder_port = responder.getPortToBind();
  const Address responder_address = Address(responder_port, family);

  parallel(
      [&]() {
        CHECK(requester.sendTo(request, responder_address));
#pragma omp barrier
#pragma omp barrier
        auto received_response = requester.receive(response.size());
        REQUIRE(received_response);
        CHECK(received_response->received_message == response);
        CHECK(are_same(received_response->sender, responder_address, family));
      },
      [&]() {
#pragma omp barrier
        auto received_request = responder.receive(request.size());
        REQUIRE(received_request);
        CHECK(received_request->received_message == request);
        CHECK(are_same(received_request->sender, requester_address, family));
        responder.sendTo(response, requester_address);
#pragma omp barrier
      });
}
