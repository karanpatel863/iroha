/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
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

#ifndef IROHA_SHARED_MODEL_BLOB_HPP
#define IROHA_SHARED_MODEL_BLOB_HPP

#include <iomanip>
#include <sstream>
#include "interfaces/model_primitive.hpp"
#include "utils/swig_keyword_hider.hpp"
#include "utils/lazy_initializer.hpp"
#include "utils/string_builder.hpp"

namespace shared_model {
  namespace crypto {

    /**
     * Blob class present user-friendly blob for working with low-level
     * binary stuff. Its length is not fixed in compile time.
     */
    class Blob : public interface::ModelPrimitive<Blob> {
     public:
      /**
       * Create blob from a string
       * @param blob - string to create blob from
       */
      explicit Blob(const std::string &blob) : blob_(blob) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (const auto &c : blob_) {
          ss << std::setw(2) << (static_cast<int>(c) & 0xff);
        }
        hex_ = ss.str();
      }

      /**
       * @return provides raw representation of blob
       */
      virtual const std::string &blob() const { return blob_; }

      /**
       * @return provides human-readable representation of blob without leading 0x
       */
      virtual const std::string &hex() const { return hex_; }

      /**
       * @return size of raw representation of blob
       */
      virtual size_t size() const { return blob_.size(); }

      std::string toString() const override {
        return detail::PrettyStringBuilder()
            .init("Blob")
            .append(hex())
            .finalize();
      }

      bool operator==(const Blob &rhs) const override {
        return blob() == rhs.blob();
      }

      Blob *copy() const override { return new Blob(blob()); };

      /**
       * Method perform transforming object to old-fashion blob_t format
       * @tparam BlobType - type of blob
       * @return blob_t array with own data
       * Design note: this method is deprecated and should be removed after
       * migration to shared model in whole project
       */

      template <typename BlobType>
      DEPRECATED BlobType makeOldModel() const {
        return BlobType::from_string(blob());
      }

     private:
      // TODO: 17/11/2017 luckychess use improved Lazy with references support
      std::string blob_;
      std::string hex_;
    };
  }  // namespace crypto
}  // namespace shared_model
#endif  // IROHA_SHARED_MODEL_BLOB_HPP