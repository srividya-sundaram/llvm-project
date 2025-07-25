// RUN: mlir-opt -allow-unregistered-dialect %s -split-input-file -verify-diagnostics | FileCheck %s

//===----------------------------------------------------------------------===//
// Test the number of regions
//===----------------------------------------------------------------------===//

func.func @correct_number_of_regions() {
    // CHECK: test.two_region_op
    "test.two_region_op"()(
      {"work"() : () -> ()},
      {"work"() : () -> ()}
    ) : () -> ()
    return
}

// -----

func.func @missing_regions() {
    // expected-error@+1 {{expected 2 regions}}
    "test.two_region_op"()(
      {"work"() : () -> ()}
    ) : () -> ()
    return
}

// -----

func.func @extra_regions() {
    // expected-error@+1 {{expected 2 regions}}
    "test.two_region_op"()(
      {"work"() : () -> ()},
      {"work"() : () -> ()},
      {"work"() : () -> ()}
    ) : () -> ()
    return
}

// -----

//===----------------------------------------------------------------------===//
// Test SizedRegion
//===----------------------------------------------------------------------===//

func.func @unnamed_region_has_wrong_number_of_blocks() {
    // expected-error@+1 {{region #1 failed to verify constraint: region with 1 blocks}}
    "test.sized_region_op"() (
    {
        "work"() : () -> ()
        cf.br ^next1
      ^next1:
        "work"() : () -> ()
    },
    {
        "work"() : () -> ()
        cf.br ^next2
      ^next2:
        "work"() : () -> ()
    }) : () -> ()
    return
}

// -----

// Test region name in error message
func.func @named_region_has_wrong_number_of_blocks() {
    // expected-error@+1 {{region #0 ('my_region') failed to verify constraint: region with 2 blocks}}
    "test.sized_region_op"() (
    {
        "work"() : () -> ()
    },
    {
        "work"() : () -> ()
    }) : () -> ()
    return
}

// -----

// Region with single block and not terminator.
// CHECK: unregistered_without_terminator
"test.unregistered_without_terminator"() ({
  ^bb0:
}) : () -> ()

// -----

// CHECK: test.single_no_terminator_op
"test.single_no_terminator_op"() (
  {
    %foo = arith.constant 1 : i32
  }
) : () -> ()

// CHECK: test.variadic_no_terminator_op
"test.variadic_no_terminator_op"() (
  {
    %foo = arith.constant 1 : i32
  },
  {
    %bar = arith.constant 1 : i32
  }
) : () -> ()

// CHECK: test.single_no_terminator_custom_asm_op
// CHECK-NEXT: important_dont_drop
test.single_no_terminator_custom_asm_op {
  "important_dont_drop"() : () -> ()
}

// -----

// CHECK: test.dummy_op_with_region_ref
test.dummy_op_with_region_ref {
  ^bb0:
}
