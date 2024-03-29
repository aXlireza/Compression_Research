/* Copyright 2022 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef XLA_TESTLIB_ATTRS
#define XLA_TESTLIB_ATTRS

include "mlir/IR/OpBase.td"
include "mlir/IR/AttrTypeBase.td"

def Dim : ArrayRefParameter<"int64_t", "Dimension">;

def PairOfDims : AttrDef<TestlibDialect, "PairOfDims"> {
  let mnemonic = "pair_of_dims";
  let summary = "Pair of dimensions";
  let parameters = (ins
      "int64_t":$rank,
      Dim:$a,
      Dim:$b
  );
  let assemblyFormat = "`<` $rank `,` `[` $a `]` `,` `[` $b `]` `>`";
}

#endif // XLA_TESTLIB_ATTRS
