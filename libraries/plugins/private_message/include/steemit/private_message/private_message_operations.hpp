
#pragma once

#include <steemit/chain/protocol/base.hpp>
#include <steemit/chain/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace steemit { namespace private_message {

struct private_message_operation : public steemit::chain::base_operation
{
    std::string                        from;
    std::string                        to;
    steemit::chain::public_key_type    from_memo_key;
    steemit::chain::public_key_type    to_memo_key;
    uint64_t                           sent_time = 0; /// used as seed to secret generation
    uint32_t                           checksum = 0;
    std::vector<char>                  encrypted_message;
};

typedef fc::static_variant< private_message_operation > operation;

} }

FC_REFLECT( steemit::private_message::private_message_operation, (from)(to)(from_memo_key)(to_memo_key)(sent_time)(checksum)(encrypted_message) )

//DECLARE_OPERATION_TYPE( steemit::private_message::operation )
FC_REFLECT_TYPENAME( steemit::private_message::operation )
