/* { dg-do compile } */
/* { dg-options "-march=rvi20u64_mafdc -mabi=lp64d" } */
#if !(defined __riscv_mul) || \
    !(defined __riscv_atomic) || \
    !(defined __riscv_flen) || \
    !(defined __riscv_div) || \
    !(defined __riscv_compressed)
#error "Feature macros not defined"
#endif
int
foo ()
{}
