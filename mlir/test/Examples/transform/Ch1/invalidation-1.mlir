// RUN: mlir-opt %s \
// RUN:   --pass-pipeline="builtin.module(transform-interpreter{ \
// RUN:        debug-bind-trailing-args=linalg.matmul,linalg.elementwise},\
// RUN:        canonicalize,cse,symbol-dce)" \
// RUN:   --split-input-file --verify-diagnostics

// ****************************** IMPORTANT NOTE ******************************
//
// If you are changing this file, you may also need to change
// mlir/docs/Tutorials/Transform accordingly.
//
// ****************************************************************************

module attributes {transform.with_named_sequence} {
  transform.named_sequence @__transform_main(
      %arg0: !transform.any_op,
      // expected-note @below {{handle to invalidated ops}}
      %arg1: !transform.op<"linalg.matmul">,
      %arg2: !transform.op<"linalg.elementwise">) {
    // The actual tiling transformation takes tile sizes as attributes.
    // expected-note @below {{invalidated by this transform op that consumes its operand #0 and invalidates all handles to payload IR entities associated with this operand and entities nested in them}}
    %tiled, %loop = transform.structured.tile_using_forall %arg1 tile_sizes [4, 32]
        : (!transform.op<"linalg.matmul">) -> (!transform.any_op, !transform.any_op)

    // This is trying to use an invalidated handle leading to undefined behavior.
    // expected-error @below {{uses a handle invalidated by a previously executed transform op}}
    transform.debug.emit_remark_at %arg1, "remark" : !transform.op<"linalg.matmul">
    transform.yield
  }
}

// Original function to optimize.
func.func @fc_relu(%lhs: tensor<512x512xf32>, %rhs: tensor<512x512xf32>,
                   %bias: tensor<512x512xf32>, %output: tensor<512x512xf32>)
                   -> tensor<512x512xf32> {
  // Matrix-matrix multiplication.
  // expected-note @below {{payload op}}
  %matmul = linalg.matmul ins(%lhs, %rhs: tensor<512x512xf32>, tensor<512x512xf32>)
                          outs(%output: tensor<512x512xf32>) -> tensor<512x512xf32>

  // Elementwise addition.
  %biased = linalg.elementwise kind=#linalg.elementwise_kind<add>
    ins(%matmul, %bias : tensor<512x512xf32>, tensor<512x512xf32>)
    outs(%output : tensor<512x512xf32>) -> tensor<512x512xf32>

  // Elementwise max with 0 (ReLU).
  %c0f = arith.constant dense<0.0> : tensor<512x512xf32>
  %relued = linalg.elementwise kind=#linalg.elementwise_kind<max_signed>
    ins(%biased, %c0f : tensor<512x512xf32>, tensor<512x512xf32>)
    outs(%output : tensor<512x512xf32>) -> tensor<512x512xf32>
  func.return %relued : tensor<512x512xf32>
}

// -----

module attributes {transform.with_named_sequence} {
  transform.named_sequence @__transform_main(
      %arg0: !transform.any_op,
      %arg1: !transform.op<"linalg.matmul">,
      %arg2: !transform.op<"linalg.elementwise">) {
    // We can cast one type to another as long as operations are compatible
    // with both types. This creates "aliasing" handles.
    // expected-note @below {{handle to invalidated ops}}
    %casted = transform.cast %arg1 : !transform.op<"linalg.matmul"> to
        !transform.any_op

    // The actual tiling transformation takes tile sizes as attributes.
    // expected-note @below {{invalidated by this transform op that consumes its operand #0 and invalidates all handles to payload IR entities associated with this operand and entities nested in them}}
    %tiled, %loop = transform.structured.tile_using_forall %arg1 tile_sizes [4, 32]
      : (!transform.op<"linalg.matmul">) -> (!transform.any_op, !transform.any_op)

    // Consuming an operand invalidates the consumed handle and any other handle that is
    // associated with the same payload operations, or payload operations nested in them.
    // expected-error @below {{uses a handle invalidated by a previously executed transform op}}
    transform.debug.emit_remark_at %casted, "remark"
      : !transform.any_op
    transform.yield
  }
}

// Original function to optimize.
func.func @fc_relu(%lhs: tensor<512x512xf32>, %rhs: tensor<512x512xf32>,
                   %bias: tensor<512x512xf32>, %output: tensor<512x512xf32>)
                   -> tensor<512x512xf32> {
  // Matrix-matrix multiplication.
  // expected-note @below {{payload op}}
  %matmul = linalg.matmul ins(%lhs, %rhs: tensor<512x512xf32>, tensor<512x512xf32>)
                          outs(%output: tensor<512x512xf32>) -> tensor<512x512xf32>

  // Elementwise addition.
  %biased = linalg.elementwise kind=#linalg.elementwise_kind<add>
    ins(%matmul, %bias : tensor<512x512xf32>, tensor<512x512xf32>)
    outs(%output : tensor<512x512xf32>) -> tensor<512x512xf32>

  // Elementwise max with 0 (ReLU).
  %c0f = arith.constant dense<0.0> : tensor<512x512xf32>
  %relued = linalg.elementwise kind=#linalg.elementwise_kind<max_signed>
    ins(%biased, %c0f : tensor<512x512xf32>, tensor<512x512xf32>)
    outs(%output : tensor<512x512xf32>) -> tensor<512x512xf32>
  func.return %relued : tensor<512x512xf32>
}
