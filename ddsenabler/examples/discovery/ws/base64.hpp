// Minimal base64 encoder — public domain.
//
// Bundled here only to encode the 20-byte SHA-1 digest into the
// Sec-WebSocket-Accept header during the WebSocket handshake (RFC 6455).

#pragma once

#include <string>

namespace eprosima {
namespace ddsenabler {
namespace examples {
namespace ws {

class Base64
{
public:

    /**
     * Base64-encode the raw bytes in @p input.
     */
    static std::string encode(
            const std::string& input)
    {
        static const char table[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string out;
        out.reserve(((input.size() + 2) / 3) * 4);

        std::size_t i = 0;
        const std::size_t n = input.size();
        while (i + 2 < n)
        {
            const uint32_t triple = (static_cast<uint8_t>(input[i]) << 16) |
                    (static_cast<uint8_t>(input[i + 1]) << 8) |
                    (static_cast<uint8_t>(input[i + 2]));
            out.push_back(table[(triple >> 18) & 0x3F]);
            out.push_back(table[(triple >> 12) & 0x3F]);
            out.push_back(table[(triple >> 6) & 0x3F]);
            out.push_back(table[triple & 0x3F]);
            i += 3;
        }

        if (i + 1 == n)
        {
            const uint32_t triple = static_cast<uint8_t>(input[i]) << 16;
            out.push_back(table[(triple >> 18) & 0x3F]);
            out.push_back(table[(triple >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
        else if (i + 2 == n)
        {
            const uint32_t triple = (static_cast<uint8_t>(input[i]) << 16) |
                    (static_cast<uint8_t>(input[i + 1]) << 8);
            out.push_back(table[(triple >> 18) & 0x3F]);
            out.push_back(table[(triple >> 12) & 0x3F]);
            out.push_back(table[(triple >> 6) & 0x3F]);
            out.push_back('=');
        }

        return out;
    }

};

} // namespace ws
} // namespace examples
} // namespace ddsenabler
} // namespace eprosima
