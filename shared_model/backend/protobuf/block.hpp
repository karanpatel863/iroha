/**
 * Copyright Soramitsu Co., Ltd. 2018 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IROHA_SHARED_MODEL_PROTO_BLOCK_HPP
#define IROHA_SHARED_MODEL_PROTO_BLOCK_HPP

#include "interfaces/iroha_internal/block.hpp"

#include "backend/protobuf/block_impl.hpp"
#include "backend/protobuf/common_objects/signature.hpp"
#include "backend/protobuf/transaction.hpp"
#include "backend/protobuf/util.hpp"
#include "block.pb.h"
#include "common_objects/abstract_noncopyable_proto.hpp"
#include "interfaces/common_objects/types.hpp"
#include "utils/lazy_initializer.hpp"

namespace shared_model {
  namespace proto {
    class Block final : public AbstractNonCopyableProto<interface::Block,
                                                        iroha::protocol::Block,
                                                        Block,
                                                        BlockImpl> {
     public:
      using AbstractNonCopyableProto::AbstractNonCopyableProto;

      Block(Block &&o) noexcept;
      Block &operator=(Block &&o) noexcept;

      interface::types::TransactionsCollectionType transactions()
          const override;

      interface::types::HeightType height() const override;

      const interface::types::HashType &prevHash() const override;

      const interface::types::BlobType &blob() const override;

      interface::types::SignatureRangeType signatures() const override;

      bool addSignature(const crypto::Signed &signed_blob,
                        const crypto::PublicKey &public_key) override;

      interface::types::TimestampType createdTime() const override;

      interface::types::TransactionsNumberType txsNumber() const override;

      const interface::types::BlobType &payload() const override;
    };
  }  // namespace proto
}  // namespace shared_model

#endif  // IROHA_SHARED_MODEL_PROTO_BLOCK_HPP
