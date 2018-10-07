/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "synchronizer/impl/synchronizer_impl.hpp"

#include <utility>

#include "ametsuchi/mutable_storage.hpp"
#include "common/visitor.hpp"
#include "interfaces/iroha_internal/block.hpp"

namespace {
  /**
   * Lambda always returning true specially for applying blocks to storage
   */
  auto trueStorageApplyPredicate = [](const auto &, auto &, const auto &) {
    return true;
  };
}  // namespace

namespace iroha {
  namespace synchronizer {

    SynchronizerImpl::SynchronizerImpl(
        std::shared_ptr<network::ConsensusGate> consensus_gate,
        std::shared_ptr<validation::ChainValidator> validator,
        std::shared_ptr<ametsuchi::MutableFactory> mutableFactory,
        std::shared_ptr<network::BlockLoader> blockLoader)
        : validator_(std::move(validator)),
          mutable_factory_(std::move(mutableFactory)),
          block_loader_(std::move(blockLoader)),
          log_(logger::log("synchronizer")) {
      consensus_gate->onOutcome().subscribe(
          subscription_, [this](network::ConsensusGate::GateObject object) {
            this->log_->info("processing consensus outcome");
            visit_in_place(object,
                           [this](const network::PairValid &msg) {
                             this->processNext(msg.block);
                           },
                           [this](const network::VoteOther &msg) {
                             this->processDifferent(msg.block);
                           },
                           [](const network::ProposalReject &msg) {
                             // nothing to do
                           },
                           [](const network::BlockReject &msg) {
                             // nothing to do
                           },
                           [this](const network::AgreementOnNone &msg) {
                             this->processNothing();
                           });
          });
    }

    std::unique_ptr<ametsuchi::MutableStorage> SynchronizerImpl::getStorage() {
      auto mutable_storage_var = mutable_factory_->createMutableStorage();
      if (auto e =
              boost::get<expected::Error<std::string>>(&mutable_storage_var)) {
        log_->error("could not create mutable storage: {}", e->error);
        return {};
      }
      return std::move(
          boost::get<
              expected::Value<std::unique_ptr<ametsuchi::MutableStorage>>>(
              &mutable_storage_var)
              ->value);
    }

    void SynchronizerImpl::processNext(
        std::shared_ptr<shared_model::interface::Block> commit_message) {
      log_->info("at handleNext");
      std::unique_ptr<ametsuchi::MutableStorage> storage = getStorage();
      if (storage == nullptr) {
        return;
      }
      storage->apply(*commit_message, trueStorageApplyPredicate);
      mutable_factory_->commit(std::move(storage));
      notifier_.get_subscriber().on_next(
          SynchronizationEvent{rxcpp::observable<>::just(commit_message),
                               SynchronizationOutcomeType::kCommit});
    }

    void SynchronizerImpl::processDifferent(
        std::shared_ptr<shared_model::interface::Block> commit_message) {
      log_->info("at handleDifferent");
      std::unique_ptr<ametsuchi::MutableStorage> storage = getStorage();
      if (storage == nullptr) {
        return;
      }
      SynchronizationEvent result;

      auto hash = commit_message->hash();

      // while blocks are not loaded and not committed
      while (1) {
        for (const auto &peer_signature : commit_message->signatures()) {
          auto network_chain = block_loader_->retrieveBlocks(
              shared_model::crypto::PublicKey(peer_signature.publicKey()));

          std::vector<std::shared_ptr<shared_model::interface::Block>> blocks;
          network_chain.as_blocking().subscribe(
              [&blocks](auto block) { blocks.push_back(block); });
          if (blocks.empty()) {
            log_->info("Downloaded an empty chain");
            continue;
          }

          auto chain =
              rxcpp::observable<>::iterate(blocks, rxcpp::identity_immediate());

          if (not(blocks.back()->hash() == hash
                  and validator_->validateChain(chain, *storage))) {
            continue;
          }

          // apply downloaded chain
          for (const auto &block : blocks) {
            // we don't need to check correctness of downloaded blocks, as
            // it was done earlier on another peer
            storage->apply(*block, trueStorageApplyPredicate);
          }
          mutable_factory_->commit(std::move(storage));

          notifier_.get_subscriber().on_next(
              SynchronizationEvent{rxcpp::observable<>::just(commit_message),
                                   SynchronizationOutcomeType::kCommit});
          return;
        }
      }
    }

    void SynchronizerImpl::processNothing() {
      log_->info("at handleNothing");
      std::unique_ptr<ametsuchi::MutableStorage> storage = getStorage();
      if (storage == nullptr) {
        return;
      }

      // Commit a stub?
    }

    rxcpp::observable<SynchronizationEvent>
    SynchronizerImpl::on_commit_chain() {
      return notifier_.get_observable();
    }

    SynchronizerImpl::~SynchronizerImpl() {
      subscription_.unsubscribe();
    }

  }  // namespace synchronizer
}  // namespace iroha
