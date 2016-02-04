#ifndef _TILE_BUILDER_H_
#define _TILE_BUILDER_H_
#include "common/kv_union.h"
#include "common/spmt.h"
#include "data/localizer.h"
#include "./tile_store.h"
namespace difacto {
/**
 * \brief helper class to build TileStore
 */
class TileBuilder {
 public:
  TileBuilder(TileStore* store, int nthreads, bool allow_multi_columns = false) {
    store_ = store;
    nthreads_ = nthreads;
    multicol_ = allow_multi_columns;
  }

  /**
   * \brief add a raw rowblk to the store
   */
  void Add(const dmlc::RowBlock<feaid_t>& rowblk,
           SArray<feaid_t>* feaids,
           SArray<real_t>* feacnts) {
    // map feature id into continous intergers
    std::shared_ptr<std::vector<feaid_t>> ids(new std::vector<feaid_t>());
    std::shared_ptr<std::vector<real_t>> cnt(new std::vector<real_t>());
    auto compacted = new dmlc::data::RowBlockContainer<unsigned>();
    Localizer lc(-1, nthreads_);
    lc.Compact(rowblk, compacted, ids.get(), feacnts ? cnt.get() : nullptr);

    // store data into tile store
    int id = blk_feaids_.size();
    if (multicol_) {
      // transpose to easy slice a column block
      auto transposed = new dmlc::data::RowBlockContainer<unsigned>();
      SpMT::Transpose(compacted->GetBlock(), transposed, ids->size(), nthreads_);
      delete compacted;

      SharedRowBlockContainer<unsigned> data(&transposed);
      data.label.CopyFrom(rowblk.label, rowblk.size);
      store_->data_->Store(std::to_string(id) + "_data", data);
      delete transposed;
    } else {
      SharedRowBlockContainer<unsigned> data(&compacted);
      store_->data_->Store(std::to_string(id) + "_data", data);
      delete compacted;
    }

    // store ids, which are used to build colmap
    SArray<feaid_t> sids(ids);
    blk_feaids_.push_back(sids);

    if (feaids) *feaids = sids;
    if (feacnts) *feacnts = SArray<real_t>(cnt);
  }

  /**
   * \brief build colmap
   * \param feaids
   */
  void BuildColmap(const SArray<real_t>& feaids,
                   const std::vector<Range>& feablk_range = {}) {
    SArray<int> map(feaids.size());
    for (size_t i = 0; i < map.size(); ++i) {
      map[i] = i+1;  // start from 1
    }

    for (size_t i = 0; i < blk_feaids_.size(); ++i) {
      // store colmap
      SArray<int> colmap;
      KVMatch(feaids, map, blk_feaids_[i], &colmap, 1, ASSIGN, nthreads_);
      for (int& c : colmap) --c;  // unmatched will get -1
      store_->data_->Store(std::to_string(i) + "_colmap", colmap);

      // store position
      if (feablk_range.size()) {
        CHECK(multicol_) << "you should set allow_multi_columns = true";
        std::vector<Range> pos;
        FindPosition(blk_feaids_[i], feablk_range, &pos);
        store_->colblk_pos_.push_back(pos);
      } else {

      }


      // clear
      blk_feaids_[i].clear();
    }
    feaids.clear();
  }
  SArray<feaid_t> feaids;
  SArray<real_t> feacnts;

  /**
   * \brief find the positionn of each feature block in the list of feature IDs
   *
   * @param feaids
   * @param feablks
   * @param positions
   */
  static void FindPosition(const SArray<feaid_t>& feaids,
                           const std::vector<Range>& feablks,
                           std::vector<Range>* positions) {
    size_t n = feablks.size();
    for (size_t i = 0; i < n; ++i) CHECK(feablks[i].Valid());
    for (size_t i = 1; i < n; ++i) CHECK_LE(feablks[i-1].end, feablks[i].begin);

    positions->resize(n);
    feaid_t const* begin = feaids.begin();
    feaid_t const* end = feaids.end();
    feaid_t const* cur = begin;

    for (size_t i = 0; i < n; ++i) {
      auto lb = std::lower_bound(cur, end, feablks[i].begin);
      auto ub = std::lower_bound(lb, end, feablks[i].end);
      cur = ub;
      positions->at(i) = Range(lb - begin, ub - begin);
    }
  }

 private:
  std::vector<SArray<feaid_t>> blk_feaids_;
  TileStore* store_;
  int nthreads_;
  bool multicol_;
};

}  // namespace difacto
#endif  // _TILE_BUILDER_H_

    // if (feaids.empty()) {
    //   feaids = sids;
    //   feacnts = scnt;
    // } else {
    //   SArray<feaid_t> new_feaids;
    //   SArray<real_t> new_feacnts;
    //   KVUnion(sids, scnt, feaids, feacnts,
    //           &new_feaids, &new_feacnts, 1, PLUS, nthreads_);
    //   feaids = new_feaids;
    //   feacnts = new_feacnts;
    // }