// RUN: %clang_cc1 -fopenmp -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -o - | FileCheck %s --check-prefix HOST --check-prefix CHECK
// RUN: %clang_cc1 -fopenmp -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm-bc %s -o %t-ppc-host.bc
// RUN: %clang_cc1 -fopenmp -x c++ -triple nvptx64-nvidia-cuda -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-target-device -fvisibility=protected -fopenmp-host-ir-file-path %t-ppc-host.bc -o - | FileCheck %s --check-prefix DEVICE --check-prefix CHECK
// RUN: %clang_cc1 -fopenmp -x c++ -triple nvptx64-nvidia-cuda -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-target-device -fvisibility=protected -fopenmp-host-ir-file-path %t-ppc-host.bc -emit-pch -o %t
// RUN: %clang_cc1 -fopenmp -x c++ -triple nvptx64-nvidia-cuda -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-target-device -fvisibility=protected -fopenmp-host-ir-file-path %t-ppc-host.bc -include-pch %t -o - | FileCheck %s --check-prefix DEVICE --check-prefix CHECK

// RUN: %clang_cc1 -fopenmp-simd -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm-bc %s -o - | FileCheck %s --check-prefix SIMD-ONLY
// RUN: %clang_cc1 -fopenmp-simd -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm-bc %s -o %t-ppc-host.bc
// RUN: %clang_cc1 -fopenmp-simd -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-target-device -fvisibility=protected -fopenmp-host-ir-file-path %t-ppc-host.bc -o -| FileCheck %s --check-prefix SIMD-ONLY
// RUN: %clang_cc1 -fopenmp-simd -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-target-device -fvisibility=protected -fopenmp-host-ir-file-path %t-ppc-host.bc -emit-pch -o %t
// RUN: %clang_cc1 -fopenmp-simd -x c++ -triple powerpc64le-unknown-unknown -fopenmp-targets=nvptx64-nvidia-cuda -emit-llvm %s -fopenmp-is-target-device -fvisibility=protected -fopenmp-host-ir-file-path %t-ppc-host.bc -include-pch %t -o - | FileCheck %s --check-prefix SIMD-ONLY

#ifndef HEADER
#define HEADER

// SIMD-ONLY-NOT: {{__kmpc|__tgt}}

// DEVICE-DAG: [[C_ADDR:.+]] = internal global i32 0,
// DEVICE-DAG: [[CD_ADDR:@.+]] ={{ protected | }}global %struct.S zeroinitializer,
// HOST-DAG: @[[C_ADDR:.+]] = internal global i32 0,
// HOST-DAG: @[[CD_ADDR:.+]] ={{( protected | dso_local)?}} global %struct.S zeroinitializer,

#pragma omp declare target
int foo() { return 0; }
#pragma omp end declare target
int bar() { return 0; }
#pragma omp declare target (bar)
int baz() { return 0; }

#pragma omp declare target
int doo() { return 0; }
#pragma omp end declare target
int car() { return 0; }
#pragma omp declare target (bar)
int caz() { return 0; }

// DEVICE-DAG: define hidden noundef i32 [[FOO:@.*foo.*]]()
// DEVICE-DAG: define hidden noundef i32 [[BAR:@.*bar.*]]()
// DEVICE-DAG: define hidden noundef i32 [[BAZ:@.*baz.*]]()
// DEVICE-DAG: define hidden noundef i32 [[DOO:@.*doo.*]]()
// DEVICE-DAG: define hidden noundef i32 [[CAR:@.*car.*]]()
// DEVICE-DAG: define hidden noundef i32 [[CAZ:@.*caz.*]]()

static int c = foo() + bar() + baz();
#pragma omp declare target (c)
// HOST-DAG: @[[C_CTOR:__omp_offloading_.+_c_l44_ctor]] = private constant i8 0
// DEVICE-DAG: define weak_odr protected void [[C_CTOR:@__omp_offloading_.+_c_l44_ctor]]()
// DEVICE-DAG: call noundef i32 [[FOO]]()
// DEVICE-DAG: call noundef i32 [[BAR]]()
// DEVICE-DAG: call noundef i32 [[BAZ]]()
// DEVICE-DAG: ret void

struct S {
  int a;
  S() = default;
  S(int a) : a(a) {}
  ~S() { a = 0; }
};

#pragma omp declare target
S cd = doo() + car() + caz() + baz();
#pragma omp end declare target
// HOST-DAG: @[[CD_CTOR:__omp_offloading_.+_cd_l61_ctor]] = private constant i8 0
// DEVICE-DAG: define weak_odr protected void [[CD_CTOR:@__omp_offloading_.+_cd_l61_ctor]]()
// DEVICE-DAG: call noundef i32 [[DOO]]()
// DEVICE-DAG: call noundef i32 [[CAR]]()
// DEVICE-DAG: call noundef i32 [[CAZ]]()
// DEVICE-DAG: ret void

// HOST-DAG: @[[CD_DTOR:__omp_offloading_.+_cd_l61_dtor]] = private constant i8 0
// DEVICE-DAG: define weak_odr protected void [[CD_DTOR:@__omp_offloading_.+_cd_l61_dtor]]()
// DEVICE-DAG: call void
// DEVICE-DAG: ret void

// HOST-DAG: @.omp_offloading.entry_name{{.*}} = internal unnamed_addr constant [{{[0-9]+}} x i8] c"[[CD_ADDR]]\00"
// HOST-DAG: @.omp_offloading.entry.[[CD_ADDR]] = weak{{.*}} constant %struct.__tgt_offload_entry { ptr @[[CD_ADDR]], ptr @.omp_offloading.entry_name{{.*}}, i64 4, i32 0, i32 0 }, section "omp_offloading_entries", align 1
// HOST-DAG: @.omp_offloading.entry_name{{.*}} = internal unnamed_addr constant [{{[0-9]+}} x i8] c"[[C_CTOR]]\00"
// HOST-DAG: @.omp_offloading.entry.[[C_CTOR]] = weak{{.*}} constant %struct.__tgt_offload_entry { ptr @[[C_CTOR]], ptr @.omp_offloading.entry_name{{.*}}, i64 0, i32 2, i32 0 }, section "omp_offloading_entries", align 1
// HOST-DAG: @.omp_offloading.entry_name{{.*}}= internal unnamed_addr constant [{{[0-9]+}} x i8] c"[[CD_CTOR]]\00"
// HOST-DAG: @.omp_offloading.entry.[[CD_CTOR]] = weak{{.*}} constant %struct.__tgt_offload_entry { ptr @[[CD_CTOR]], ptr @.omp_offloading.entry_name{{.*}}, i64 0, i32 2, i32 0 }, section "omp_offloading_entries", align 1
// HOST-DAG: @.omp_offloading.entry_name{{.*}}= internal unnamed_addr constant [{{[0-9]+}} x i8] c"[[CD_DTOR]]\00"
// HOST-DAG: @.omp_offloading.entry.[[CD_DTOR]] = weak{{.*}} constant %struct.__tgt_offload_entry { ptr @[[CD_DTOR]], ptr @.omp_offloading.entry_name{{.*}}, i64 0, i32 4, i32 0 }, section "omp_offloading_entries", align 1
int maini1() {
  int a;
#pragma omp target map(tofrom : a)
  {
    a = c;
  }
  return 0;
}

// DEVICE-DAG: define weak{{.*}} void @__omp_offloading_{{.*}}_{{.*}}maini1{{.*}}_l[[@LINE-7]](ptr {{[^,]*}}, ptr noundef nonnull align {{[0-9]+}} dereferenceable{{[^,]*}}
// DEVICE-DAG: [[C:%.+]] = load i32, ptr [[C_ADDR]],
// DEVICE-DAG: store i32 [[C]], ptr %

// HOST: define internal void @__omp_offloading_{{.*}}_{{.*}}maini1{{.*}}_l[[@LINE-11]](ptr noundef nonnull align {{[0-9]+}} dereferenceable{{.*}})
// HOST: [[C:%.*]] = load i32, ptr @[[C_ADDR]],
// HOST: store i32 [[C]], ptr %

// HOST-DAG: !{i32 1, !"[[CD_ADDR]]", i32 0, i32 {{[0-9]+}}}
// HOST-DAG: !{i32 1, !"[[C_ADDR]]", i32 0, i32 {{[0-9]+}}}

// DEVICE: !nvvm.annotations
// DEVICE-DAG: !{ptr [[C_CTOR]], !"kernel", i32 1}
// DEVICE-DAG: !{ptr [[CD_CTOR]], !"kernel", i32 1}
// DEVICE-DAG: !{ptr [[CD_DTOR]], !"kernel", i32 1}

#endif // HEADER

