#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "util/result.hpp"

// Forward declare TorrentFile
namespace fur {
struct TorrentFile;
}

/// Contains data structures and facilities for representing and discovering
/// BitTorrent peers
namespace fur::peer {

/// Represents a single peer as given by the tracker
struct Peer {
  uint32_t ip;
  uint16_t port;

  /// Constructs an empty 'Peer'
  explicit Peer();

  /// Constructs a `Peer` given an IP and a port
  Peer(uint32_t ip, uint16_t port);

  /// Constructs a `Peer` given an IP string and a port
  Peer(const std::string& ip, uint16_t port);

  /// Combines the ip and port of the peer into a X.Y.Z.W:PORT string
  /// \return The combined string
  [[nodiscard]] std::string address() const;
};

/// The response sent from the tracker when announcing
struct Announce {
  /// How often (in seconds) we're expected to re-announce ourselves and refresh
  /// the list of peers
  int interval;
  /// The list of peers that we can download the file from
  std::vector<Peer> peers;
};

enum class PeerError {
  /// Generic error that comes from the BencodeParser
  ParserError,
  /// Can't announce to the tracker
  AnnounceError
};

using PeerResult = util::Result<Announce, PeerError>;

/// Announce ourselves to the tracker and get a list of peers to download the
/// file from
[[nodiscard]] PeerResult announce(const TorrentFile& torrent_f);

}  // namespace fur::peer
