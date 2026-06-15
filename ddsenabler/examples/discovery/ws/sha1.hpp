// SHA-1 implementation — public domain.
//
// Adapted from the public-domain "100% free public domain implementation of the
// SHA-1 algorithm" by Steve Reid et al. Trimmed to the single-shot digest needed
// by the WebSocket handshake (RFC 6455). It is NOT a cryptographically reviewed
// implementation and is bundled here only to compute the Sec-WebSocket-Accept key
// for this example; do not reuse it for security-sensitive purposes.

#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace eprosima {
namespace ddsenabler {
namespace examples {
namespace ws {

class Sha1
{
public:

    /**
     * Compute the SHA-1 digest of @p input.
     *
     * @return the 20-byte raw digest as a std::string (binary, not hex).
     */
    static std::string digest(
            const std::string& input)
    {
        uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};

        // Pre-processing: append the bit '1', then padding zeros, then the 64-bit length.
        std::string msg = input;
        const uint64_t bit_len = static_cast<uint64_t>(msg.size()) * 8u;

        msg.push_back(static_cast<char>(0x80));
        while ((msg.size() % 64u) != 56u)
        {
            msg.push_back(static_cast<char>(0x00));
        }
        for (int i = 7; i >= 0; --i)
        {
            msg.push_back(static_cast<char>((bit_len >> (i * 8)) & 0xFFu));
        }

        // Process each 512-bit (64-byte) chunk.
        for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64u)
        {
            uint32_t w[80];
            for (int i = 0; i < 16; ++i)
            {
                w[i] = (static_cast<uint8_t>(msg[chunk + i * 4 + 0]) << 24) |
                        (static_cast<uint8_t>(msg[chunk + i * 4 + 1]) << 16) |
                        (static_cast<uint8_t>(msg[chunk + i * 4 + 2]) << 8) |
                        (static_cast<uint8_t>(msg[chunk + i * 4 + 3]));
            }
            for (int i = 16; i < 80; ++i)
            {
                w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
            }

            uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

            for (int i = 0; i < 80; ++i)
            {
                uint32_t f, k;
                if (i < 20)
                {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999u;
                }
                else if (i < 40)
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1u;
                }
                else if (i < 60)
                {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDCu;
                }
                else
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6u;
                }

                const uint32_t tmp = rotl(a, 5) + f + e + k + w[i];
                e = d;
                d = c;
                c = rotl(b, 30);
                b = a;
                a = tmp;
            }

            h[0] += a;
            h[1] += b;
            h[2] += c;
            h[3] += d;
            h[4] += e;
        }

        std::string out;
        out.resize(20);
        for (int i = 0; i < 5; ++i)
        {
            out[i * 4 + 0] = static_cast<char>((h[i] >> 24) & 0xFFu);
            out[i * 4 + 1] = static_cast<char>((h[i] >> 16) & 0xFFu);
            out[i * 4 + 2] = static_cast<char>((h[i] >> 8) & 0xFFu);
            out[i * 4 + 3] = static_cast<char>(h[i] & 0xFFu);
        }
        return out;
    }

private:

    static uint32_t rotl(
            uint32_t value,
            int bits)
    {
        return (value << bits) | (value >> (32 - bits));
    }

};

} // namespace ws
} // namespace examples
} // namespace ddsenabler
} // namespace eprosima
