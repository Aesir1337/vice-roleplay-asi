#include "viceasi/actions.hpp"
#include "viceasi/bitstream_utils.hpp"

#include "RakHook/rakhook.hpp"
#include "RakNet/BitStream.h"
#include "RakNet/PacketEnumerations.h"

namespace vice::actions {

namespace {
constexpr unsigned char RPC_SET_PLAYER_NAME = 11;
}

void change_name() {
    RakNet::BitStream rpc;
    rpc.Write<unsigned short>(0);
    utils::write_with_size<unsigned char>(&rpc, "test_name");
    rpc.Write<unsigned char>(1);

    rakhook::emul_rpc(RPC_SET_PLAYER_NAME, rpc);
}

void emul_player_sync() {
    RakNet::BitStream bs;
    float             pos[3]  = {0};
    float             quat[4] = {0};

    bs.Write<unsigned char>(ID_PLAYER_SYNC);
    bs.Write<unsigned short>(0);
    bs.Write0();
    bs.Write0();
    bs.Write<unsigned short>(0);
    bs.Write(pos);
    bs.Write(quat);
    bs.Write<unsigned char>(255);
    bs.Write<unsigned char>(0);
    bs.Write<unsigned char>(0);
    bs.Write<float>(0);
    bs.Write0();
    bs.Write0();

    rakhook::emul_packet(bs);
}

} // namespace vice::actions
