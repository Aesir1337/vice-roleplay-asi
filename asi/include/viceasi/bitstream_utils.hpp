#ifndef VICEASI_BITSTREAM_UTILS_HPP
#define VICEASI_BITSTREAM_UTILS_HPP

#include <string>
#include <string_view>

#include "RakNet/BitStream.h"

namespace vice::utils {

template <typename SizeT>
std::string read_with_size(RakNet::BitStream *bs) {
    SizeT size;
    if (!bs->Read(size))
        return {};
    std::string str(size, '\0');
    bs->Read(str.data(), size);
    return str;
}

template <typename SizeT>
void write_with_size(RakNet::BitStream *bs, std::string_view str) {
    SizeT size = static_cast<SizeT>(str.size());
    bs->Write(size);
    bs->Write(str.data(), size);
}

} // namespace vice::utils

#endif // VICEASI_BITSTREAM_UTILS_HPP
