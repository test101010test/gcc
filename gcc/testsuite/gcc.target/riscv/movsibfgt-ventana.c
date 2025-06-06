/* { dg-do compile { target { ! riscv_abi_e } } } */
/* { dg-require-effective-target rv64 } */
/* { dg-skip-if "" { *-*-* } { "-O0" "-Og" "-Os" "-Oz" } } */
/* { dg-options "-march=rv64gc_xventanacondops -mtune=rocket -mbranch-cost=2 -fdump-rtl-ce1" } */

typedef int __attribute__ ((mode (SI))) int_t;

int_t
movsifgt (double w, double x, int_t y, int_t z)
{
  return w > x ? y : z;
}

/* Expect branched assembly like:

	fgt.d	a4,fa0,fa1
	mv	a5,a0
	mv	a0,a1
	beq	a4,zero,.L2
	mv	a0,a5
.L2:
 */

/* { dg-final { scan-rtl-dump-not "Conversion succeeded on pass \[0-9\]+\\." "ce1" } } */
/* { dg-final { scan-rtl-dump-not "if-conversion succeeded through" "ce1" } } */
/* { dg-final { scan-assembler-times "\\s(?:fgt\\.d|flt\\.d)\\s" 1 } } */
/* { dg-final { scan-assembler-times "\\s(?:beq|bne)\\s" 1 } } */
/* { dg-final { scan-assembler-not "\\svt\\.maskc\\s" } } */
/* { dg-final { scan-assembler-not "\\svt\\.maskcn\\s" } } */
/* { dg-final { scan-assembler-not "\\s(?:seqz|snez)\\s" } } */
