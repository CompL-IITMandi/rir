; ModuleID = '98_95.bc'

%R_bcstack_t = type { i32, i32, %struct.SEXPREC* }
%struct.SEXPREC = type { %struct.sxpinfo_struct, %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*, %union.SEXP_SEXP_SEXP }
%struct.sxpinfo_struct = type { i64 }
%union.SEXP_SEXP_SEXP = type { %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC* }

@spe_BCNodeStackTop = available_externally externally_initialized global %R_bcstack_t*
@sym_a = available_externally externally_initialized global %struct.SEXPREC
@dcs_107 = available_externally externally_initialized global %struct.SEXPREC
@0 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@dcs_106 = available_externally externally_initialized global %struct.SEXPREC
@1 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@spe_Visible = available_externally externally_initialized global i32

define %struct.SEXPREC* @hastFun_1_98(i8* %code, %R_bcstack_t* %args, %struct.SEXPREC* %env, %struct.SEXPREC* %closure) {
  %1 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %2 = alloca %struct.SEXPREC*, i64 0, align 8
  %"PIR%0.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %"PIR%0.2" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %3 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %4 = bitcast %R_bcstack_t* %3 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %4, i8 0, i64 16, i1 false)
  %5 = getelementptr %R_bcstack_t, %R_bcstack_t* %3, i32 1
  store %R_bcstack_t* %5, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  br label %BB0

BB0:                                              ; preds = %0
  %a = call %struct.SEXPREC* @ldvarGlobal(%struct.SEXPREC* @sym_a)
  %6 = icmp eq %struct.SEXPREC* %a, @dcs_107
  br i1 %6, label %9, label %7, !prof !0

7:                                                ; preds = %9, %BB0
  %8 = icmp eq %struct.SEXPREC* %a, @dcs_106
  br i1 %8, label %16, label %10, !prof !0

9:                                                ; preds = %BB0
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @0, i32 0, i32 0))
  br label %7

10:                                               ; preds = %16, %7
  store %struct.SEXPREC* %a, %struct.SEXPREC** %"PIR%0.0", align 8
  store i32 1, i32* @spe_Visible, align 4
  %11 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%0.0", align 8
  %sxpinfo = getelementptr %struct.SEXPREC, %struct.SEXPREC* %11, i32 0, i32 0, i32 0
  %12 = load i64, i64* %sxpinfo, align 4
  %13 = and i64 %12, 31
  %14 = trunc i64 %13 to i32
  %15 = icmp eq i32 %14, 5
  br i1 %15, label %17, label %23

16:                                               ; preds = %7
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @1, i32 0, i32 0))
  br label %10

17:                                               ; preds = %10
  %18 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %11, i32 0, i32 4, i32 0
  %19 = load %struct.SEXPREC*, %struct.SEXPREC** %18, align 8
  %20 = icmp eq %struct.SEXPREC* %19, @dcs_106
  br i1 %20, label %21, label %24, !prof !1

21:                                               ; preds = %17
  %22 = call %struct.SEXPREC* @forcePromise(%struct.SEXPREC* %11)
  br label %25

23:                                               ; preds = %10
  br label %25

24:                                               ; preds = %17
  br label %25

25:                                               ; preds = %24, %23, %21
  %"PIR%0.21" = phi %struct.SEXPREC* [ %22, %21 ], [ %11, %23 ], [ %19, %24 ]
  store %struct.SEXPREC* %"PIR%0.21", %struct.SEXPREC** %"PIR%0.2", align 8
  %26 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%0.2", align 8
  %27 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %28 = getelementptr %R_bcstack_t, %R_bcstack_t* %27, i32 -1
  store %R_bcstack_t* %28, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  ret %struct.SEXPREC* %26
}

declare %struct.SEXPREC* @ldvarGlobal(%struct.SEXPREC*)

declare void @error(i8*)

declare %struct.SEXPREC* @forcePromise(%struct.SEXPREC*)

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #0

attributes #0 = { argmemonly nounwind willreturn }

!0 = !{!"branch_weights", i32 1, i32 100000000}
!1 = !{!"branch_weights", i32 1, i32 1000}
