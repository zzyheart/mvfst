/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

#pragma once

#include <fizz/server/ServerExtensions.h>
#include <quic/fizz/handshake/FizzTransportParameters.h>
#include <quic/server/handshake/StatelessResetGenerator.h>

namespace quic {

class ServerTransportParametersExtension : public fizz::ServerExtensions {
 public:
  ServerTransportParametersExtension(
      QuicVersion encodingVersion,
      uint64_t initialMaxData,
      uint64_t initialMaxStreamDataBidiLocal,
      uint64_t initialMaxStreamDataBidiRemote,
      uint64_t initialMaxStreamDataUni,
      uint64_t initialMaxStreamsBidi,
      uint64_t initialMaxStreamsUni,
      std::chrono::milliseconds idleTimeout,
      uint64_t ackDelayExponent,
      uint64_t maxRecvPacketSize,
      TransportPartialReliabilitySetting partialReliability,
      const StatelessResetToken& token)
      : encodingVersion_(encodingVersion),
        initialMaxData_(initialMaxData),
        initialMaxStreamDataBidiLocal_(initialMaxStreamDataBidiLocal),
        initialMaxStreamDataBidiRemote_(initialMaxStreamDataBidiRemote),
        initialMaxStreamDataUni_(initialMaxStreamDataUni),
        initialMaxStreamsBidi_(initialMaxStreamsBidi),
        initialMaxStreamsUni_(initialMaxStreamsUni),
        idleTimeout_(idleTimeout),
        ackDelayExponent_(ackDelayExponent),
        maxRecvPacketSize_(maxRecvPacketSize),
        partialReliability_(partialReliability),
        token_(token) {}

  ~ServerTransportParametersExtension() override = default;

  std::vector<fizz::Extension> getExtensions(
      const fizz::ClientHello& chlo) override {
    auto clientParams =
        fizz::getClientExtension(chlo.extensions, encodingVersion_);

    if (!clientParams) {
      throw fizz::FizzException(
          "missing client quic transport parameters extension",
          fizz::AlertDescription::missing_extension);
    }
    clientTransportParameters_ = std::move(clientParams);

    std::vector<fizz::Extension> exts;

    ServerTransportParameters params;
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::initial_max_stream_data_bidi_local,
        initialMaxStreamDataBidiLocal_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::initial_max_stream_data_bidi_remote,
        initialMaxStreamDataBidiRemote_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::initial_max_stream_data_uni,
        initialMaxStreamDataUni_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::initial_max_data, initialMaxData_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::initial_max_streams_bidi,
        initialMaxStreamsBidi_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::initial_max_streams_uni, initialMaxStreamsUni_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::idle_timeout, idleTimeout_.count()));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::ack_delay_exponent, ackDelayExponent_));
    params.parameters.push_back(encodeIntegerParameter(
        TransportParameterId::max_packet_size, maxRecvPacketSize_));
    TransportParameter statelessReset;
    statelessReset.parameter = TransportParameterId::stateless_reset_token;
    statelessReset.value = folly::IOBuf::copyBuffer(token_);
    params.parameters.push_back(std::move(statelessReset));

    uint64_t partialReliabilitySetting = 0;
    if (partialReliability_) {
      partialReliabilitySetting = 1;
    }
    params.parameters.push_back(encodeIntegerParameter(
        static_cast<TransportParameterId>(kPartialReliabilityParameterId),
        partialReliabilitySetting));

    exts.push_back(encodeExtension(params, encodingVersion_));
    return exts;
  }

  folly::Optional<ClientTransportParameters> getClientTransportParams() {
    return std::move(clientTransportParameters_);
  }

 private:
  QuicVersion encodingVersion_;
  uint64_t initialMaxData_;
  uint64_t initialMaxStreamDataBidiLocal_;
  uint64_t initialMaxStreamDataBidiRemote_;
  uint64_t initialMaxStreamDataUni_;
  uint64_t initialMaxStreamsBidi_;
  uint64_t initialMaxStreamsUni_;
  std::chrono::milliseconds idleTimeout_;
  uint64_t ackDelayExponent_;
  uint64_t maxRecvPacketSize_;
  TransportPartialReliabilitySetting partialReliability_;
  folly::Optional<ClientTransportParameters> clientTransportParameters_;
  StatelessResetToken token_;
};
} // namespace quic
