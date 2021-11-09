; ModuleID = '367_4296032527.bc'

%R_bcstack_t = type { i32, i32, %struct.SEXPREC* }
%struct.SEXPREC = type { %struct.sxpinfo_struct, %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*, %union.SEXP_SEXP_SEXP }
%struct.sxpinfo_struct = type { i64 }
%union.SEXP_SEXP_SEXP = type { %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC* }
%DeoptReason = type <{ i32, i32, i8* }>
%struct.VECTOR_SEXPREC = type { %struct.sxpinfo_struct, %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*, %struct.vecsxp_struct }
%struct.vecsxp_struct = type { i64, i64 }
%LazyEnvironment = type { i32, i32, i32, i64, i8* }

@spe_BCNodeStackTop = available_externally externally_initialized global %R_bcstack_t*
@spe_constantPool = available_externally externally_initialized global i64
@sym_b = available_externally externally_initialized global %struct.SEXPREC
@dcs_107 = available_externally externally_initialized global %struct.SEXPREC
@0 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@dcs_106 = available_externally externally_initialized global %struct.SEXPREC
@1 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@dcs_104 = available_externally externally_initialized global %struct.SEXPREC
@dcs_111 = available_externally externally_initialized global %struct.SEXPREC
@dcs_100 = available_externally externally_initialized global %struct.SEXPREC
@copool_4 = private constant [1 x i32] zeroinitializer
@epe_367_0_4296032527 = available_externally externally_initialized global i8
@hast_367 = available_externally externally_initialized global i8
@copool_5 = private constant %DeoptReason <{ i32 1, i32 228, i8* @hast_367 }>

define %struct.SEXPREC* @f0_0_16F_10010410F(i8* %code, %R_bcstack_t* %args, %struct.SEXPREC* %env, %struct.SEXPREC* %closure) {
  %1 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %2 = alloca %struct.SEXPREC*, i64 0, align 8
  %"PIR%0.2" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %"PIR%4.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %"PIR%4.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %PIRe5.0 = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %3 = getelementptr %R_bcstack_t, %R_bcstack_t* %args, i32 0, i32 2
  %4 = load %struct.SEXPREC*, %struct.SEXPREC** %3, align 8
  %5 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %6 = bitcast %R_bcstack_t* %5 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %6, i8 0, i64 64, i1 false)
  %7 = getelementptr %R_bcstack_t, %R_bcstack_t* %5, i32 4
  store %R_bcstack_t* %7, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  br label %BB0

BB0:                                              ; preds = %0
  %sxpinfo = getelementptr %struct.SEXPREC, %struct.SEXPREC* %4, i32 0, i32 0, i32 0
  %8 = load i64, i64* %sxpinfo, align 4
  %9 = and i64 %8, 31
  %10 = trunc i64 %9 to i32
  %11 = icmp eq i32 %10, 5
  br i1 %11, label %isProm, label %14, !prof !0

isProm:                                           ; preds = %BB0
  %12 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %4, i32 0, i32 4, i32 0
  %13 = load %struct.SEXPREC*, %struct.SEXPREC** %12, align 8
  br label %15

14:                                               ; preds = %BB0
  br label %15

15:                                               ; preds = %14, %isProm
  %16 = phi %struct.SEXPREC* [ %13, %isProm ], [ %4, %14 ]
  %sxpinfo1 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %16, i32 0, i32 0, i32 0
  %17 = load i64, i64* %sxpinfo1, align 4
  %18 = and i64 %17, 31
  %19 = trunc i64 %18 to i32
  %20 = icmp eq i32 %19, 14
  br i1 %20, label %isReal, label %notReal

21:                                               ; preds = %32, %isNa, %isReal
  %"PIR%0.1" = phi double [ %33, %32 ], [ 0x7FF8000000000000, %isNa ], [ %26, %isReal ]
  %b = call %struct.SEXPREC* @ldvarGlobal(%struct.SEXPREC* @sym_b)
  %22 = icmp eq %struct.SEXPREC* %b, @dcs_107
  br i1 %22, label %36, label %34, !prof !1

isReal:                                           ; preds = %15
  %23 = bitcast %struct.SEXPREC* %16 to %struct.VECTOR_SEXPREC*
  %24 = getelementptr %struct.VECTOR_SEXPREC, %struct.VECTOR_SEXPREC* %23, i32 1
  %25 = bitcast %struct.VECTOR_SEXPREC* %24 to double*
  %26 = load double, double* %25, align 8
  br label %21

notReal:                                          ; preds = %15
  %27 = bitcast %struct.SEXPREC* %16 to %struct.VECTOR_SEXPREC*
  %28 = getelementptr %struct.VECTOR_SEXPREC, %struct.VECTOR_SEXPREC* %27, i32 1
  %29 = bitcast %struct.VECTOR_SEXPREC* %28 to i32*
  %30 = load i32, i32* %29, align 4
  %31 = icmp ne i32 %30, -2147483648
  br i1 %31, label %32, label %isNa, !prof !2

isNa:                                             ; preds = %notReal
  br label %21

32:                                               ; preds = %notReal
  %33 = sitofp i32 %30 to double
  br label %21

34:                                               ; preds = %36, %21
  %35 = icmp eq %struct.SEXPREC* %b, @dcs_106
  br i1 %35, label %55, label %37, !prof !1

36:                                               ; preds = %21
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @0, i32 0, i32 0))
  br label %34

37:                                               ; preds = %55, %34
  store %struct.SEXPREC* %b, %struct.SEXPREC** %"PIR%0.2", align 8
  %38 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%0.2", align 8
  %sxpinfo2 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %38, i32 0, i32 0, i32 0
  %39 = load i64, i64* %sxpinfo2, align 4
  %40 = and i64 %39, 31
  %41 = trunc i64 %40 to i32
  %42 = icmp eq i32 %41, 14
  %43 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %38, i32 0, i32 1
  %44 = load %struct.SEXPREC*, %struct.SEXPREC** %43, align 8
  %45 = icmp eq %struct.SEXPREC* %44, @dcs_104
  %46 = and i1 %42, %45
  %47 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %38, i32 0, i32 1
  %48 = load %struct.SEXPREC*, %struct.SEXPREC** %47, align 8
  %49 = icmp eq %struct.SEXPREC* %48, @dcs_104
  %sxpinfo3 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %38, i32 0, i32 0, i32 0
  %50 = load i64, i64* %sxpinfo3, align 4
  %51 = and i64 %50, 64
  %52 = icmp ne i64 0, %51
  %53 = xor i1 %52, true
  %54 = and i1 %53, %49
  br i1 %54, label %64, label %56

55:                                               ; preds = %34
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @1, i32 0, i32 0))
  br label %37

56:                                               ; preds = %37
  %57 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %48, i32 0, i32 4, i32 2
  %58 = load %struct.SEXPREC*, %struct.SEXPREC** %57, align 8
  %59 = icmp eq %struct.SEXPREC* %58, @dcs_111
  %60 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %48, i32 0, i32 4, i32 1
  %61 = load %struct.SEXPREC*, %struct.SEXPREC** %60, align 8
  %62 = icmp eq %struct.SEXPREC* %61, @dcs_104
  %63 = and i1 %59, %62
  br label %64

64:                                               ; preds = %56, %37
  %65 = phi i1 [ true, %37 ], [ %63, %56 ]
  %66 = and i1 %46, %65
  %"PIR%0.3" = zext i1 %66 to i32
  %67 = icmp ne i32 %"PIR%0.3", 0
  br i1 %67, label %BB4, label %BB5, !prof !3

BB4:                                              ; preds = %64
  %"PIR%4.04" = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%0.2", align 8
  store %struct.SEXPREC* %"PIR%4.04", %struct.SEXPREC** %"PIR%4.0", align 8
  %68 = call %struct.SEXPREC* @newReal(double %"PIR%0.1")
  %69 = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  store volatile %struct.SEXPREC* %68, %struct.SEXPREC** %69, align 8
  %70 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%4.0", align 8
  %"PIR%4.15" = call %struct.SEXPREC* @binop(%struct.SEXPREC* %68, %struct.SEXPREC* %70, i32 0)
  store %struct.SEXPREC* %"PIR%4.15", %struct.SEXPREC** %"PIR%4.1", align 8
  %71 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%4.1", align 8
  %72 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %73 = getelementptr %R_bcstack_t, %R_bcstack_t* %72, i32 -4
  store %R_bcstack_t* %73, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  ret %struct.SEXPREC* %71

BB5:                                              ; preds = %64
  %PIRe5.09 = call %struct.SEXPREC* @createStubEnvironment(%struct.SEXPREC* @dcs_100, i32 1, i32* getelementptr inbounds ([1 x i32], [1 x i32]* @copool_4, i32 0, i32 0), i32 1)
  %74 = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  store volatile %struct.SEXPREC* %PIRe5.09, %struct.SEXPREC** %74, align 8
  %sxpinfo6 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %PIRe5.09, i32 0, i32 0, i32 0
  %75 = load i64, i64* %sxpinfo6, align 4
  %76 = and i64 %75, 16777216
  %77 = icmp ne i64 %76, 0
  br i1 %77, label %92, label %83

78:                                               ; preds = %100, %83
  %sxpinfo8 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %4, i32 0, i32 0, i32 0
  %79 = load i64, i64* %sxpinfo8, align 4
  %80 = lshr i64 %79, 32
  %81 = and i64 %80, 65535
  %82 = icmp eq i64 %81, 7
  br i1 %82, label %106, label %101

83:                                               ; preds = %96, %BB5
  %84 = bitcast %struct.SEXPREC* %PIRe5.09 to %struct.VECTOR_SEXPREC*
  %85 = getelementptr %struct.VECTOR_SEXPREC, %struct.VECTOR_SEXPREC* %84, i32 1
  %86 = bitcast %struct.VECTOR_SEXPREC* %85 to %LazyEnvironment*
  %87 = getelementptr %LazyEnvironment, %LazyEnvironment* %86, i32 1
  %88 = bitcast %LazyEnvironment* %87 to i8*
  %89 = getelementptr i8, i8* %88, i64 1
  %90 = bitcast i8* %89 to %struct.SEXPREC**
  %91 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %90, i32 2
  store %struct.SEXPREC* %4, %struct.SEXPREC** %91, align 8
  br label %78

92:                                               ; preds = %BB5
  %sxpinfo7 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %4, i32 0, i32 0, i32 0
  %93 = load i64, i64* %sxpinfo7, align 4
  %94 = and i64 %93, 16777216
  %95 = icmp ne i64 %94, 0
  br i1 %95, label %96, label %100

96:                                               ; preds = %92
  %97 = and i64 %75, 268435456
  %98 = and i64 %93, 268435456
  %99 = icmp ugt i64 %97, %98
  br i1 %99, label %100, label %83, !prof !0

100:                                              ; preds = %96, %92
  call void @externalsxpSetEntry(%struct.SEXPREC* %PIRe5.09, i32 2, %struct.SEXPREC* %4)
  br label %78

101:                                              ; preds = %78
  %102 = add nuw nsw i64 %81, 1
  %103 = shl i64 %102, 32
  %104 = and i64 %79, -281470681743361
  %105 = or i64 %104, %103
  store i64 %105, i64* %sxpinfo8, align 4
  br label %106

106:                                              ; preds = %101, %78
  store %struct.SEXPREC* %PIRe5.09, %struct.SEXPREC** %PIRe5.0, align 8
  %107 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %108 = getelementptr %R_bcstack_t, %R_bcstack_t* %107, i32 2
  store %R_bcstack_t* %108, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %109 = call %struct.SEXPREC* @newReal(double %"PIR%0.1")
  %110 = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 3, i32 2
  store volatile %struct.SEXPREC* %109, %struct.SEXPREC** %110, align 8
  %111 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe5.0, align 8
  %112 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %113 = getelementptr %R_bcstack_t, %R_bcstack_t* %112, i64 -2
  %114 = bitcast %R_bcstack_t* %113 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %114, i8 0, i64 32, i1 false)
  %115 = getelementptr %R_bcstack_t, %R_bcstack_t* %112, i64 -2, i32 2
  store %struct.SEXPREC* %109, %struct.SEXPREC** %115, align 8
  %116 = getelementptr %R_bcstack_t, %R_bcstack_t* %112, i64 -1, i32 2
  store %struct.SEXPREC* %111, %struct.SEXPREC** %116, align 8
  %117 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%0.2", align 8
  call void @deopt(i8* %code, %struct.SEXPREC* %closure, i8* @epe_367_0_4296032527, %R_bcstack_t* %args, %DeoptReason* @copool_5, %struct.SEXPREC* %117)
  %118 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %119 = getelementptr %R_bcstack_t, %R_bcstack_t* %118, i32 -2
  store %R_bcstack_t* %119, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  unreachable
}

declare %struct.SEXPREC* @ldvarGlobal(%struct.SEXPREC*)

declare void @error(i8*)

declare %struct.SEXPREC* @newReal(double)

declare %struct.SEXPREC* @binop(%struct.SEXPREC*, %struct.SEXPREC*, i32)

declare %struct.SEXPREC* @createStubEnvironment(%struct.SEXPREC*, i32, i32*, i32)

declare void @externalsxpSetEntry(%struct.SEXPREC*, i32, %struct.SEXPREC*)

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #0

declare void @deopt(i8*, %struct.SEXPREC*, i8*, %R_bcstack_t*, %DeoptReason*, %struct.SEXPREC*)

attributes #0 = { argmemonly nounwind willreturn }

!0 = !{!"branch_weights", i32 1, i32 1000}
!1 = !{!"branch_weights", i32 1, i32 100000000}
!2 = !{!"branch_weights", i32 1000, i32 1}
!3 = !{!"branch_weights", i32 100000000, i32 1}
