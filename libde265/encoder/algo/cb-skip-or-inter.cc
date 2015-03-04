/*
 * H.265 video codec.
 * Copyright (c) 2013-2014 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * Authors: struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This file is part of libde265.
 *
 * libde265 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libde265 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libde265.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "libde265/encoder/algo/cb-skip-or-inter.h"
#include "libde265/encoder/encoder-context.h"
#include <assert.h>
#include <limits>
#include <math.h>




enc_cb* Algo_CB_SkipOrInter_BruteForce::analyze(encoder_context* ectx,
                                                context_model_table ctxModel,
                                                enc_cb* cb)
{
#if 0
  CodingOptions options(ectx);
  options.set_input(cb,ctxModel);
  options.activate_option(0, try_skip);
  options.activate_option(1, try_inter);

  if (try_skip) {
    enc_cb* cb = options[0];
    cb->PredMode = MODE_SKIP;
    ectx->img->set_pred_mode(cb->x,cb->y, cb->log2Size, cb->PredMode);

    options.begin_reconstruction(0);
    cb_skip = mSkipAlgo->analyze(ectx,
                                 options.get_context(0),
                                 options.get_cb(0));
    options.end_reconstruction();
  }

  if (try_inter) {
    enc_cb* cb = options[1];

    options.begin_reconstruction(1);
    cb_inter = mInterAlgo->analyze(ectx,
                                   options.get_context(1),
                                   options.get_cb(1));
    options.end_reconstruction();
  }

  return options.return_best_rdo();
#endif




  // if we try both variants, make a copy of the ctxModel and use the copy for splitting

  bool try_skip  = true;
  bool try_inter = false; // TODO

  context_model_table ctxInter;

  if (try_inter) {
    copy_context_model_table(ctxInter, ctxModel);
  }


  // try encoding with inter

  enc_cb* cb_skip  = NULL;
  enc_cb* cb_inter = NULL;

  if (try_inter) {
    cb_inter = new enc_cb;
    *cb_inter = *cb;

    cb_inter = mInterAlgo->analyze(ectx, ctxInter, cb_inter);
  }


  // try skip

  if (try_skip) {
    cb_skip = new enc_cb;
    *cb_skip = *cb;

    cb_skip->PredMode = MODE_SKIP;
    ectx->img->set_pred_mode(cb->x,cb->y, cb->log2Size, cb->PredMode);

    cb_skip = mSkipAlgo->analyze(ectx, ctxModel, cb_skip);
  }

  delete cb;


  // if only one variant has been tested, choose this

  assert(cb_skip || cb_inter);

  if (!try_inter) { return cb_skip;  }
  if (!try_skip)  { return cb_inter; }


  // compute RD costs for both variants

  const float rd_cost_inter = cb_inter->distortion + ectx->lambda * cb_inter->rate;
  const float rd_cost_skip  = cb_skip ->distortion + ectx->lambda * cb_skip ->rate;

  const bool inter_is_better =  (rd_cost_inter < rd_cost_skip);

  if (inter_is_better) {
    copy_context_model_table(ctxModel, ctxInter);

    // have to reconstruct state
    //cb_inter->write_to_image(ectx->img);
    cb_inter->reconstruct(ectx, ectx->img);
    return cb_inter;
  }
  else {
    return cb_skip;
  }
}
