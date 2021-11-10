; ModuleID = '7966_25777260047.bc'

%R_bcstack_t = type { i32, i32, %struct.SEXPREC* }
%struct.SEXPREC = type { %struct.sxpinfo_struct, %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*, %union.SEXP_SEXP_SEXP }
%struct.sxpinfo_struct = type { i64 }
%union.SEXP_SEXP_SEXP = type { %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC* }
%DeoptReason = type <{ i32, i32, i8* }>
%struct.VECTOR_SEXPREC = type { %struct.sxpinfo_struct, %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*, %struct.vecsxp_struct }
%struct.vecsxp_struct = type { i64, i64 }

@spe_BCNodeStackTop = available_externally externally_initialized global %R_bcstack_t*
@spe_constantPool = available_externally externally_initialized global i64
@dcs_106 = available_externally externally_initialized global %struct.SEXPREC
@dcs_102 = available_externally externally_initialized global %struct.SEXPREC
@copool_14 = private constant [3 x i32] [i32 0, i32 1, i32 2]
@dcs_104 = available_externally externally_initialized global %struct.SEXPREC
@sym_stop = available_externally externally_initialized global %struct.SEXPREC
@sym_start = available_externally externally_initialized global %struct.SEXPREC
@sym_x = available_externally externally_initialized global %struct.SEXPREC
@sym_as.character = available_externally externally_initialized global %struct.SEXPREC
@dcs_107 = available_externally externally_initialized global %struct.SEXPREC
@0 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@1 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@gcb_317 = available_externally externally_initialized global %struct.SEXPREC
@2 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@3 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@4 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@5 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@sym_as.integer = available_externally externally_initialized global %struct.SEXPREC
@6 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@7 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@gcb_318 = available_externally externally_initialized global %struct.SEXPREC
@8 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@9 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@dcs_111 = available_externally externally_initialized global %struct.SEXPREC
@dcs_101 = available_externally externally_initialized global %struct.SEXPREC
@10 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@11 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@12 = private unnamed_addr constant [37 x i8] c"argument is missing, with no default\00", align 1
@13 = private unnamed_addr constant [17 x i8] c"object not found\00", align 1
@copool_15 = private constant i32 3
@spe_Visible = available_externally externally_initialized global i32
@gcb_340 = available_externally externally_initialized global %struct.SEXPREC
@epe_7966_0_25777260047 = available_externally externally_initialized global i8
@hast_7966 = available_externally externally_initialized global i8
@copool_16 = private constant %DeoptReason <{ i32 2, i32 353, i8* @hast_7966 }>
@epe_7966_1_25777260047 = available_externally externally_initialized global i8
@copool_17 = private constant %DeoptReason <{ i32 6, i32 468, i8* @hast_7966 }>
@dcs_103 = available_externally externally_initialized global %struct.SEXPREC
@dcs_105 = available_externally externally_initialized global %struct.SEXPREC
@epe_7966_2_25777260047 = available_externally externally_initialized global i8
@copool_18 = private constant %DeoptReason <{ i32 1, i32 537, i8* @hast_7966 }>
@epe_7966_3_25777260047 = available_externally externally_initialized global i8
@copool_19 = private constant %DeoptReason <{ i32 6, i32 576, i8* @hast_7966 }>
@epe_7966_4_25777260047 = available_externally externally_initialized global i8
@copool_20 = private constant %DeoptReason <{ i32 1, i32 645, i8* @hast_7966 }>

define %struct.SEXPREC* @f0_0_1F1E_60071C60F(i8* %code, %R_bcstack_t* %args, %struct.SEXPREC* %env, %struct.SEXPREC* %closure) {
  %1 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %2 = alloca %struct.SEXPREC*, i64 5, align 8
  %"PIR%0.3" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %PIRe0.5 = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 0, i32 2
  %"PIR%3.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%3.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%3.2" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  %"PIR%15.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  %"PIR%15.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  %"PIR%17.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  %"PIR%17.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 2, i32 2
  %"PIR%17.2" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 3, i32 2
  %"PIR%12.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%19.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 3, i32 2
  %"PIR%19.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 3, i32 2
  %"PIR%13.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%13.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%13.2" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%14.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %"PIR%21.0" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 3, i32 2
  %"PIR%21.1" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 3, i32 2
  %"PIR%21.2" = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 1, i32 2
  %3 = getelementptr %R_bcstack_t, %R_bcstack_t* %args, i32 0, i32 2
  %4 = load %struct.SEXPREC*, %struct.SEXPREC** %3, align 8
  %5 = getelementptr %R_bcstack_t, %R_bcstack_t* %args, i32 1, i32 2
  %6 = load %struct.SEXPREC*, %struct.SEXPREC** %5, align 8
  %7 = getelementptr %R_bcstack_t, %R_bcstack_t* %args, i32 2, i32 2
  %8 = load %struct.SEXPREC*, %struct.SEXPREC** %7, align 8
  %9 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %10 = bitcast %R_bcstack_t* %9 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %10, i8 0, i64 80, i1 false)
  %11 = getelementptr %R_bcstack_t, %R_bcstack_t* %9, i32 5
  store %R_bcstack_t* %11, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  br label %BB0

BB0:                                              ; preds = %0
  %sxpinfo = getelementptr %struct.SEXPREC, %struct.SEXPREC* %4, i32 0, i32 0, i32 0
  %12 = load i64, i64* %sxpinfo, align 4
  %13 = and i64 %12, 31
  %14 = trunc i64 %13 to i32
  %15 = icmp eq i32 %14, 5
  br i1 %15, label %16, label %22

16:                                               ; preds = %BB0
  %17 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %4, i32 0, i32 4, i32 0
  %18 = load %struct.SEXPREC*, %struct.SEXPREC** %17, align 8
  %19 = icmp eq %struct.SEXPREC* %18, @dcs_106
  br i1 %19, label %20, label %23, !prof !0

20:                                               ; preds = %16
  %21 = call %struct.SEXPREC* @forcePromise(%struct.SEXPREC* %4)
  br label %24

22:                                               ; preds = %BB0
  br label %24

23:                                               ; preds = %16
  br label %24

24:                                               ; preds = %23, %22, %20
  %"PIR%0.31" = phi %struct.SEXPREC* [ %21, %20 ], [ %4, %22 ], [ %18, %23 ]
  store %struct.SEXPREC* %"PIR%0.31", %struct.SEXPREC** %"PIR%0.3", align 8
  %25 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%0.3", align 8
  %sxpinfo2 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %25, i32 0, i32 0, i32 0
  %26 = load i64, i64* %sxpinfo2, align 4
  %27 = and i64 %26, 31
  %28 = trunc i64 %27 to i32
  %29 = icmp eq i32 %28, 16
  %"PIR%0.4" = select i1 %29, i32 1, i32 0
  %30 = call %struct.SEXPREC* @createBindingCellImpl(%struct.SEXPREC* %8, %struct.SEXPREC* @sym_stop, %struct.SEXPREC* @dcs_104)
  %31 = call %struct.SEXPREC* @createBindingCellImpl(%struct.SEXPREC* %6, %struct.SEXPREC* @sym_start, %struct.SEXPREC* %30)
  %32 = call %struct.SEXPREC* @createBindingCellImpl(%struct.SEXPREC* %4, %struct.SEXPREC* @sym_x, %struct.SEXPREC* %31)
  %PIRe0.53 = call %struct.SEXPREC* @createEnvironment(%struct.SEXPREC* @dcs_102, %struct.SEXPREC* %32, i32 1)
  store %struct.SEXPREC* %PIRe0.53, %struct.SEXPREC** %PIRe0.5, align 8
  %33 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 4
  store %struct.SEXPREC* null, %struct.SEXPREC** %33, align 8
  %34 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 3
  store %struct.SEXPREC* null, %struct.SEXPREC** %34, align 8
  %35 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 2
  store %struct.SEXPREC* null, %struct.SEXPREC** %35, align 8
  %36 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 1
  store %struct.SEXPREC* null, %struct.SEXPREC** %36, align 8
  %37 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 0
  store %struct.SEXPREC* null, %struct.SEXPREC** %37, align 8
  %38 = icmp ne i32 %"PIR%0.4", 0
  br i1 %38, label %BB2, label %BB12

BB2:                                              ; preds = %24
  br label %BB3

BB12:                                             ; preds = %24
  %39 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 3
  %40 = load %struct.SEXPREC*, %struct.SEXPREC** %39, align 8
  %41 = ptrtoint %struct.SEXPREC* %40 to i64
  %42 = icmp ule i64 %41, 2
  br i1 %42, label %51, label %43, !prof !0

43:                                               ; preds = %BB12
  %44 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %40, i32 0, i32 4, i32 0
  %45 = load %struct.SEXPREC*, %struct.SEXPREC** %44, align 8
  %46 = icmp eq %struct.SEXPREC* %45, @dcs_106
  br i1 %46, label %51, label %47, !prof !0

47:                                               ; preds = %43
  %sxpinfo4 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %45, i32 0, i32 0, i32 0
  %48 = load i64, i64* %sxpinfo4, align 4
  %49 = and i64 %48, 281470681743360
  %50 = icmp eq i64 %49, 0
  br i1 %50, label %notNamed, label %57

51:                                               ; preds = %43, %BB12
  %52 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %53 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_as.character, %struct.SEXPREC* %52, %struct.SEXPREC** %39)
  br label %54

54:                                               ; preds = %57, %51
  %as.character = phi %struct.SEXPREC* [ %45, %57 ], [ %53, %51 ]
  %55 = icmp eq %struct.SEXPREC* %as.character, @dcs_107
  br i1 %55, label %60, label %58, !prof !1

notNamed:                                         ; preds = %47
  %56 = or i64 %48, 4294967296
  store i64 %56, i64* %sxpinfo4, align 4
  br label %57

57:                                               ; preds = %notNamed, %47
  br label %54

58:                                               ; preds = %60, %54
  %59 = icmp eq %struct.SEXPREC* %as.character, @dcs_106
  br i1 %59, label %67, label %61, !prof !1

60:                                               ; preds = %54
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @0, i32 0, i32 0))
  br label %58

61:                                               ; preds = %67, %58
  store %struct.SEXPREC* %as.character, %struct.SEXPREC** %"PIR%12.0", align 8
  %62 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%12.0", align 8
  %sxpinfo5 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %62, i32 0, i32 0, i32 0
  %63 = load i64, i64* %sxpinfo5, align 4
  %64 = and i64 %63, 31
  %65 = trunc i64 %64 to i32
  %66 = icmp eq i32 %65, 5
  br i1 %66, label %isProm, label %70, !prof !0

67:                                               ; preds = %58
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @1, i32 0, i32 0))
  br label %61

isProm:                                           ; preds = %61
  %68 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %62, i32 0, i32 4, i32 0
  %69 = load %struct.SEXPREC*, %struct.SEXPREC** %68, align 8
  br label %71

70:                                               ; preds = %61
  br label %71

71:                                               ; preds = %70, %isProm
  %72 = phi %struct.SEXPREC* [ %69, %isProm ], [ %62, %70 ]
  %73 = icmp eq %struct.SEXPREC* @gcb_317, %72
  %74 = icmp ne %struct.SEXPREC* %72, @dcs_106
  %75 = and i1 %73, %74
  br i1 %75, label %90, label %76

76:                                               ; preds = %71
  %77 = load i64, i64* getelementptr inbounds (%struct.SEXPREC, %struct.SEXPREC* @gcb_317, i32 0, i32 0, i32 0), align 4
  %78 = and i64 %77, 31
  %79 = trunc i64 %78 to i32
  %80 = icmp eq i32 %79, 3
  %sxpinfo6 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %72, i32 0, i32 0, i32 0
  %81 = load i64, i64* %sxpinfo6, align 4
  %82 = and i64 %81, 31
  %83 = trunc i64 %82 to i32
  %84 = icmp eq i32 %83, 3
  %85 = and i1 %80, %84
  br i1 %85, label %86, label %88

86:                                               ; preds = %76
  %87 = call i1 @cksEq(%struct.SEXPREC* @gcb_317, %struct.SEXPREC* %72)
  br label %88

88:                                               ; preds = %86, %76
  %89 = phi i1 [ %87, %86 ], [ false, %76 ]
  br label %90

90:                                               ; preds = %88, %71
  %91 = phi i1 [ true, %71 ], [ %89, %88 ]
  %"PIR%12.1" = zext i1 %91 to i32
  %92 = icmp ne i32 %"PIR%12.1", 0
  br i1 %92, label %BB13, label %BB14, !prof !2

BB13:                                             ; preds = %90
  %93 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 0
  %94 = load %struct.SEXPREC*, %struct.SEXPREC** %93, align 8
  %95 = ptrtoint %struct.SEXPREC* %94 to i64
  %96 = icmp ule i64 %95, 2
  br i1 %96, label %118, label %110, !prof !0

BB14:                                             ; preds = %90
  %97 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %"PIR%14.050" = call %struct.SEXPREC* @ldfun(%struct.SEXPREC* @sym_as.character, %struct.SEXPREC* %97)
  store %struct.SEXPREC* %"PIR%14.050", %struct.SEXPREC** %"PIR%14.0", align 8
  store i32 1, i32* @spe_Visible, align 4
  %98 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %99 = getelementptr %R_bcstack_t, %R_bcstack_t* %98, i32 2
  store %R_bcstack_t* %99, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %100 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%14.0", align 8
  %101 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %102 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %103 = getelementptr %R_bcstack_t, %R_bcstack_t* %102, i64 -2
  %104 = bitcast %R_bcstack_t* %103 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %104, i8 0, i64 32, i1 false)
  %105 = getelementptr %R_bcstack_t, %R_bcstack_t* %102, i64 -2, i32 2
  store %struct.SEXPREC* %100, %struct.SEXPREC** %105, align 8
  %106 = getelementptr %R_bcstack_t, %R_bcstack_t* %102, i64 -1, i32 2
  store %struct.SEXPREC* %101, %struct.SEXPREC** %106, align 8
  %107 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%14.0", align 8
  call void @deopt(i8* %code, %struct.SEXPREC* %closure, i8* @epe_7966_0_25777260047, %R_bcstack_t* %args, %DeoptReason* @copool_16, %struct.SEXPREC* %107)
  %108 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %109 = getelementptr %R_bcstack_t, %R_bcstack_t* %108, i32 -2
  store %R_bcstack_t* %109, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  unreachable

110:                                              ; preds = %BB13
  %111 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %94, i32 0, i32 4, i32 0
  %112 = load %struct.SEXPREC*, %struct.SEXPREC** %111, align 8
  %113 = icmp eq %struct.SEXPREC* %112, @dcs_106
  br i1 %113, label %118, label %114, !prof !0

114:                                              ; preds = %110
  %sxpinfo7 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %112, i32 0, i32 0, i32 0
  %115 = load i64, i64* %sxpinfo7, align 4
  %116 = and i64 %115, 281470681743360
  %117 = icmp eq i64 %116, 0
  br i1 %117, label %notNamed8, label %124

118:                                              ; preds = %110, %BB13
  %119 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %120 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_x, %struct.SEXPREC* %119, %struct.SEXPREC** %93)
  br label %121

121:                                              ; preds = %124, %118
  %x = phi %struct.SEXPREC* [ %112, %124 ], [ %120, %118 ]
  %122 = icmp eq %struct.SEXPREC* %x, @dcs_107
  br i1 %122, label %127, label %125, !prof !1

notNamed8:                                        ; preds = %114
  %123 = or i64 %115, 4294967296
  store i64 %123, i64* %sxpinfo7, align 4
  br label %124

124:                                              ; preds = %notNamed8, %114
  br label %121

125:                                              ; preds = %127, %121
  %126 = icmp eq %struct.SEXPREC* %x, @dcs_106
  br i1 %126, label %134, label %128, !prof !1

127:                                              ; preds = %121
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @2, i32 0, i32 0))
  br label %125

128:                                              ; preds = %134, %125
  store %struct.SEXPREC* %x, %struct.SEXPREC** %"PIR%13.0", align 8
  %129 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%13.0", align 8
  %sxpinfo9 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %129, i32 0, i32 0, i32 0
  %130 = load i64, i64* %sxpinfo9, align 4
  %131 = and i64 %130, 31
  %132 = trunc i64 %131 to i32
  %133 = icmp eq i32 %132, 5
  br i1 %133, label %135, label %141

134:                                              ; preds = %125
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @3, i32 0, i32 0))
  br label %128

135:                                              ; preds = %128
  %136 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %129, i32 0, i32 4, i32 0
  %137 = load %struct.SEXPREC*, %struct.SEXPREC** %136, align 8
  %138 = icmp eq %struct.SEXPREC* %137, @dcs_106
  br i1 %138, label %139, label %142, !prof !0

139:                                              ; preds = %135
  %140 = call %struct.SEXPREC* @forcePromise(%struct.SEXPREC* %129)
  br label %143

141:                                              ; preds = %128
  br label %143

142:                                              ; preds = %135
  br label %143

143:                                              ; preds = %142, %141, %139
  %"PIR%13.110" = phi %struct.SEXPREC* [ %140, %139 ], [ %129, %141 ], [ %137, %142 ]
  store %struct.SEXPREC* %"PIR%13.110", %struct.SEXPREC** %"PIR%13.1", align 8
  %144 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %145 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %146 = getelementptr %R_bcstack_t, %R_bcstack_t* %145, i32 1
  store %R_bcstack_t* %146, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %147 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%13.1", align 8
  %148 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %149 = getelementptr %R_bcstack_t, %R_bcstack_t* %148, i64 -1
  %150 = bitcast %R_bcstack_t* %149 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %150, i8 0, i64 16, i1 false)
  %151 = getelementptr %R_bcstack_t, %R_bcstack_t* %148, i64 -1, i32 2
  store %struct.SEXPREC* %147, %struct.SEXPREC** %151, align 8
  %"PIR%13.211" = call %struct.SEXPREC* @callBuiltin(i8* %code, i32 7, %struct.SEXPREC* @gcb_317, %struct.SEXPREC* %144, i64 1)
  %152 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %153 = getelementptr %R_bcstack_t, %R_bcstack_t* %152, i32 -1
  store %R_bcstack_t* %153, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  store %struct.SEXPREC* %"PIR%13.211", %struct.SEXPREC** %"PIR%13.2", align 8
  %154 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 0
  %155 = load %struct.SEXPREC*, %struct.SEXPREC** %154, align 8
  %156 = ptrtoint %struct.SEXPREC* %155 to i64
  %157 = icmp ule i64 %156, 2
  br i1 %157, label %174, label %158, !prof !0

158:                                              ; preds = %143
  %159 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %155, i32 0, i32 4, i32 0
  %160 = load %struct.SEXPREC*, %struct.SEXPREC** %159, align 8
  %161 = icmp eq %struct.SEXPREC* %160, @dcs_106
  br i1 %161, label %174, label %162, !prof !0

162:                                              ; preds = %158
  %163 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%13.2", align 8
  %164 = icmp eq %struct.SEXPREC* %160, %163
  br i1 %164, label %170, label %165, !prof !0

165:                                              ; preds = %162
  %sxpinfo12 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %163, i32 0, i32 0, i32 0
  %166 = load i64, i64* %sxpinfo12, align 4
  %167 = lshr i64 %166, 32
  %168 = and i64 %167, 65535
  %169 = icmp eq i64 %168, 7
  br i1 %169, label %183, label %178

170:                                              ; preds = %162
  %sxpinfo15 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %160, i32 0, i32 0, i32 0
  %171 = load i64, i64* %sxpinfo15, align 4
  %172 = and i64 %171, 281470681743360
  %173 = icmp eq i64 %172, 0
  br i1 %173, label %notNamed16, label %205

174:                                              ; preds = %158, %143
  %175 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%13.2", align 8
  %176 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  call void @stvar(%struct.SEXPREC* @sym_x, %struct.SEXPREC* %175, %struct.SEXPREC* %176)
  br label %177

177:                                              ; preds = %205, %187, %174
  br label %BB3

178:                                              ; preds = %165
  %179 = add nuw nsw i64 %168, 1
  %180 = shl i64 %179, 32
  %181 = and i64 %166, -281470681743361
  %182 = or i64 %181, %180
  store i64 %182, i64* %sxpinfo12, align 4
  br label %183

183:                                              ; preds = %178, %165
  %sxpinfo13 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %155, i32 0, i32 0, i32 0
  %184 = load i64, i64* %sxpinfo13, align 4
  %185 = and i64 %184, 16777216
  %186 = icmp ne i64 %185, 0
  br i1 %186, label %190, label %188

187:                                              ; preds = %202, %188
  br label %177

188:                                              ; preds = %194, %183
  %189 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %155, i32 0, i32 4, i32 0
  store %struct.SEXPREC* %163, %struct.SEXPREC** %189, align 8
  br label %187

190:                                              ; preds = %183
  %sxpinfo14 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %163, i32 0, i32 0, i32 0
  %191 = load i64, i64* %sxpinfo14, align 4
  %192 = and i64 %191, 16777216
  %193 = icmp ne i64 %192, 0
  br i1 %193, label %194, label %198

194:                                              ; preds = %190
  %195 = and i64 %184, 268435456
  %196 = and i64 %191, 268435456
  %197 = icmp ugt i64 %195, %196
  br i1 %197, label %198, label %188, !prof !0

198:                                              ; preds = %194, %190
  %199 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %155, i32 0, i32 4, i32 0
  %200 = load %struct.SEXPREC*, %struct.SEXPREC** %199, align 8
  %201 = icmp ne %struct.SEXPREC* %200, %163
  br i1 %201, label %203, label %202

202:                                              ; preds = %203, %198
  br label %187

203:                                              ; preds = %198
  call void @setCar(%struct.SEXPREC* %155, %struct.SEXPREC* %163)
  br label %202

notNamed16:                                       ; preds = %170
  %204 = or i64 %171, 4294967296
  store i64 %204, i64* %sxpinfo15, align 4
  br label %205

205:                                              ; preds = %notNamed16, %170
  br label %177

BB3:                                              ; preds = %177, %BB2
  %206 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 0
  %207 = load %struct.SEXPREC*, %struct.SEXPREC** %206, align 8
  %208 = ptrtoint %struct.SEXPREC* %207 to i64
  %209 = icmp ule i64 %208, 2
  br i1 %209, label %218, label %210, !prof !0

210:                                              ; preds = %BB3
  %211 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %207, i32 0, i32 4, i32 0
  %212 = load %struct.SEXPREC*, %struct.SEXPREC** %211, align 8
  %213 = icmp eq %struct.SEXPREC* %212, @dcs_106
  br i1 %213, label %218, label %214, !prof !0

214:                                              ; preds = %210
  %sxpinfo17 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %212, i32 0, i32 0, i32 0
  %215 = load i64, i64* %sxpinfo17, align 4
  %216 = and i64 %215, 281470681743360
  %217 = icmp eq i64 %216, 0
  br i1 %217, label %notNamed18, label %224

218:                                              ; preds = %210, %BB3
  %219 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %220 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_x, %struct.SEXPREC* %219, %struct.SEXPREC** %206)
  br label %221

221:                                              ; preds = %224, %218
  %x19 = phi %struct.SEXPREC* [ %212, %224 ], [ %220, %218 ]
  %222 = icmp eq %struct.SEXPREC* %x19, @dcs_107
  br i1 %222, label %227, label %225, !prof !1

notNamed18:                                       ; preds = %214
  %223 = or i64 %215, 4294967296
  store i64 %223, i64* %sxpinfo17, align 4
  br label %224

224:                                              ; preds = %notNamed18, %214
  br label %221

225:                                              ; preds = %227, %221
  %226 = icmp eq %struct.SEXPREC* %x19, @dcs_106
  br i1 %226, label %234, label %228, !prof !1

227:                                              ; preds = %221
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @4, i32 0, i32 0))
  br label %225

228:                                              ; preds = %234, %225
  store %struct.SEXPREC* %x19, %struct.SEXPREC** %"PIR%3.0", align 8
  %229 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.0", align 8
  %sxpinfo20 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %229, i32 0, i32 0, i32 0
  %230 = load i64, i64* %sxpinfo20, align 4
  %231 = and i64 %230, 31
  %232 = trunc i64 %231 to i32
  %233 = icmp eq i32 %232, 5
  br i1 %233, label %235, label %241

234:                                              ; preds = %225
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @5, i32 0, i32 0))
  br label %228

235:                                              ; preds = %228
  %236 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %229, i32 0, i32 4, i32 0
  %237 = load %struct.SEXPREC*, %struct.SEXPREC** %236, align 8
  %238 = icmp eq %struct.SEXPREC* %237, @dcs_106
  br i1 %238, label %239, label %242, !prof !0

239:                                              ; preds = %235
  %240 = call %struct.SEXPREC* @forcePromise(%struct.SEXPREC* %229)
  br label %243

241:                                              ; preds = %228
  br label %243

242:                                              ; preds = %235
  br label %243

243:                                              ; preds = %242, %241, %239
  %"PIR%3.121" = phi %struct.SEXPREC* [ %240, %239 ], [ %229, %241 ], [ %237, %242 ]
  store %struct.SEXPREC* %"PIR%3.121", %struct.SEXPREC** %"PIR%3.1", align 8
  %244 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 1
  %245 = load %struct.SEXPREC*, %struct.SEXPREC** %244, align 8
  %246 = ptrtoint %struct.SEXPREC* %245 to i64
  %247 = icmp ule i64 %246, 2
  br i1 %247, label %256, label %248, !prof !0

248:                                              ; preds = %243
  %249 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %245, i32 0, i32 4, i32 0
  %250 = load %struct.SEXPREC*, %struct.SEXPREC** %249, align 8
  %251 = icmp eq %struct.SEXPREC* %250, @dcs_106
  br i1 %251, label %256, label %252, !prof !0

252:                                              ; preds = %248
  %sxpinfo22 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %250, i32 0, i32 0, i32 0
  %253 = load i64, i64* %sxpinfo22, align 4
  %254 = and i64 %253, 281470681743360
  %255 = icmp eq i64 %254, 0
  br i1 %255, label %notNamed23, label %262

256:                                              ; preds = %248, %243
  %257 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %258 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_as.integer, %struct.SEXPREC* %257, %struct.SEXPREC** %244)
  br label %259

259:                                              ; preds = %262, %256
  %as.integer = phi %struct.SEXPREC* [ %250, %262 ], [ %258, %256 ]
  %260 = icmp eq %struct.SEXPREC* %as.integer, @dcs_107
  br i1 %260, label %265, label %263, !prof !1

notNamed23:                                       ; preds = %252
  %261 = or i64 %253, 4294967296
  store i64 %261, i64* %sxpinfo22, align 4
  br label %262

262:                                              ; preds = %notNamed23, %252
  br label %259

263:                                              ; preds = %265, %259
  %264 = icmp eq %struct.SEXPREC* %as.integer, @dcs_106
  br i1 %264, label %272, label %266, !prof !1

265:                                              ; preds = %259
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @6, i32 0, i32 0))
  br label %263

266:                                              ; preds = %272, %263
  store %struct.SEXPREC* %as.integer, %struct.SEXPREC** %"PIR%3.2", align 8
  %267 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.2", align 8
  %sxpinfo25 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %267, i32 0, i32 0, i32 0
  %268 = load i64, i64* %sxpinfo25, align 4
  %269 = and i64 %268, 31
  %270 = trunc i64 %269 to i32
  %271 = icmp eq i32 %270, 5
  br i1 %271, label %isProm24, label %275, !prof !0

272:                                              ; preds = %263
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @7, i32 0, i32 0))
  br label %266

isProm24:                                         ; preds = %266
  %273 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %267, i32 0, i32 4, i32 0
  %274 = load %struct.SEXPREC*, %struct.SEXPREC** %273, align 8
  br label %276

275:                                              ; preds = %266
  br label %276

276:                                              ; preds = %275, %isProm24
  %277 = phi %struct.SEXPREC* [ %274, %isProm24 ], [ %267, %275 ]
  %278 = icmp eq %struct.SEXPREC* @gcb_318, %277
  %279 = icmp ne %struct.SEXPREC* %277, @dcs_106
  %280 = and i1 %278, %279
  br i1 %280, label %295, label %281

281:                                              ; preds = %276
  %282 = load i64, i64* getelementptr inbounds (%struct.SEXPREC, %struct.SEXPREC* @gcb_318, i32 0, i32 0, i32 0), align 4
  %283 = and i64 %282, 31
  %284 = trunc i64 %283 to i32
  %285 = icmp eq i32 %284, 3
  %sxpinfo26 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %277, i32 0, i32 0, i32 0
  %286 = load i64, i64* %sxpinfo26, align 4
  %287 = and i64 %286, 31
  %288 = trunc i64 %287 to i32
  %289 = icmp eq i32 %288, 3
  %290 = and i1 %285, %289
  br i1 %290, label %291, label %293

291:                                              ; preds = %281
  %292 = call i1 @cksEq(%struct.SEXPREC* @gcb_318, %struct.SEXPREC* %277)
  br label %293

293:                                              ; preds = %291, %281
  %294 = phi i1 [ %292, %291 ], [ false, %281 ]
  br label %295

295:                                              ; preds = %293, %276
  %296 = phi i1 [ true, %276 ], [ %294, %293 ]
  %"PIR%3.3" = zext i1 %296 to i32
  %297 = icmp ne i32 %"PIR%3.3", 0
  br i1 %297, label %BB15, label %BB16, !prof !2

BB15:                                             ; preds = %295
  %298 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 2
  %299 = load %struct.SEXPREC*, %struct.SEXPREC** %298, align 8
  %300 = ptrtoint %struct.SEXPREC* %299 to i64
  %301 = icmp ule i64 %300, 2
  br i1 %301, label %323, label %315, !prof !0

BB16:                                             ; preds = %295
  %302 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %303 = getelementptr %R_bcstack_t, %R_bcstack_t* %302, i32 2
  store %R_bcstack_t* %303, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %304 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.1", align 8
  %305 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %306 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %307 = getelementptr %R_bcstack_t, %R_bcstack_t* %306, i64 -2
  %308 = bitcast %R_bcstack_t* %307 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %308, i8 0, i64 32, i1 false)
  %309 = getelementptr %R_bcstack_t, %R_bcstack_t* %306, i64 -2, i32 2
  store %struct.SEXPREC* %304, %struct.SEXPREC** %309, align 8
  %310 = getelementptr %R_bcstack_t, %R_bcstack_t* %306, i64 -1, i32 2
  store %struct.SEXPREC* %305, %struct.SEXPREC** %310, align 8
  %311 = icmp ne i32 %"PIR%3.3", 0
  %312 = select i1 %311, %struct.SEXPREC* @dcs_103, %struct.SEXPREC* @dcs_105
  call void @deopt(i8* %code, %struct.SEXPREC* %closure, i8* @epe_7966_1_25777260047, %R_bcstack_t* %args, %DeoptReason* @copool_17, %struct.SEXPREC* %312)
  %313 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %314 = getelementptr %R_bcstack_t, %R_bcstack_t* %313, i32 -2
  store %R_bcstack_t* %314, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  unreachable

315:                                              ; preds = %BB15
  %316 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %299, i32 0, i32 4, i32 0
  %317 = load %struct.SEXPREC*, %struct.SEXPREC** %316, align 8
  %318 = icmp eq %struct.SEXPREC* %317, @dcs_106
  br i1 %318, label %323, label %319, !prof !0

319:                                              ; preds = %315
  %sxpinfo27 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %317, i32 0, i32 0, i32 0
  %320 = load i64, i64* %sxpinfo27, align 4
  %321 = and i64 %320, 281470681743360
  %322 = icmp eq i64 %321, 0
  br i1 %322, label %notNamed28, label %329

323:                                              ; preds = %315, %BB15
  %324 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %325 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_start, %struct.SEXPREC* %324, %struct.SEXPREC** %298)
  br label %326

326:                                              ; preds = %329, %323
  %start = phi %struct.SEXPREC* [ %317, %329 ], [ %325, %323 ]
  %327 = icmp eq %struct.SEXPREC* %start, @dcs_107
  br i1 %327, label %332, label %330, !prof !1

notNamed28:                                       ; preds = %319
  %328 = or i64 %320, 4294967296
  store i64 %328, i64* %sxpinfo27, align 4
  br label %329

329:                                              ; preds = %notNamed28, %319
  br label %326

330:                                              ; preds = %332, %326
  %331 = icmp eq %struct.SEXPREC* %start, @dcs_106
  br i1 %331, label %339, label %333, !prof !1

332:                                              ; preds = %326
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @8, i32 0, i32 0))
  br label %330

333:                                              ; preds = %339, %330
  store %struct.SEXPREC* %start, %struct.SEXPREC** %"PIR%15.0", align 8
  %334 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%15.0", align 8
  %sxpinfo29 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %334, i32 0, i32 0, i32 0
  %335 = load i64, i64* %sxpinfo29, align 4
  %336 = and i64 %335, 31
  %337 = trunc i64 %336 to i32
  %338 = icmp eq i32 %337, 5
  br i1 %338, label %340, label %346

339:                                              ; preds = %330
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @9, i32 0, i32 0))
  br label %333

340:                                              ; preds = %333
  %341 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %334, i32 0, i32 4, i32 0
  %342 = load %struct.SEXPREC*, %struct.SEXPREC** %341, align 8
  %343 = icmp eq %struct.SEXPREC* %342, @dcs_106
  br i1 %343, label %344, label %347, !prof !0

344:                                              ; preds = %340
  %345 = call %struct.SEXPREC* @forcePromise(%struct.SEXPREC* %334)
  br label %348

346:                                              ; preds = %333
  br label %348

347:                                              ; preds = %340
  br label %348

348:                                              ; preds = %347, %346, %344
  %"PIR%15.130" = phi %struct.SEXPREC* [ %345, %344 ], [ %334, %346 ], [ %342, %347 ]
  store %struct.SEXPREC* %"PIR%15.130", %struct.SEXPREC** %"PIR%15.1", align 8
  %349 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%15.1", align 8
  %350 = icmp ne %struct.SEXPREC* %349, @dcs_106
  %351 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %349, i32 0, i32 1
  %352 = load %struct.SEXPREC*, %struct.SEXPREC** %351, align 8
  %353 = icmp eq %struct.SEXPREC* %352, @dcs_104
  %354 = and i1 %350, %353
  %355 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %349, i32 0, i32 1
  %356 = load %struct.SEXPREC*, %struct.SEXPREC** %355, align 8
  %357 = icmp eq %struct.SEXPREC* %356, @dcs_104
  %sxpinfo31 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %349, i32 0, i32 0, i32 0
  %358 = load i64, i64* %sxpinfo31, align 4
  %359 = and i64 %358, 64
  %360 = icmp ne i64 0, %359
  %361 = xor i1 %360, true
  %362 = and i1 %361, %357
  br i1 %362, label %371, label %363

363:                                              ; preds = %348
  %364 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %356, i32 0, i32 4, i32 2
  %365 = load %struct.SEXPREC*, %struct.SEXPREC** %364, align 8
  %366 = icmp eq %struct.SEXPREC* %365, @dcs_111
  %367 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %356, i32 0, i32 4, i32 1
  %368 = load %struct.SEXPREC*, %struct.SEXPREC** %367, align 8
  %369 = icmp eq %struct.SEXPREC* %368, @dcs_104
  %370 = and i1 %366, %369
  br label %371

371:                                              ; preds = %363, %348
  %372 = phi i1 [ true, %348 ], [ %370, %363 ]
  %373 = and i1 %354, %372
  %"PIR%15.2" = zext i1 %373 to i32
  %374 = icmp ne i32 %"PIR%15.2", 0
  br i1 %374, label %BB17, label %BB18, !prof !2

BB17:                                             ; preds = %371
  %"PIR%17.032" = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%15.1", align 8
  store %struct.SEXPREC* %"PIR%17.032", %struct.SEXPREC** %"PIR%17.0", align 8
  %375 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%17.0", align 8
  %376 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %375, i32 0, i32 1
  %377 = load %struct.SEXPREC*, %struct.SEXPREC** %376, align 8
  %378 = icmp eq %struct.SEXPREC* %377, @dcs_104
  %sxpinfo33 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %375, i32 0, i32 0, i32 0
  %379 = load i64, i64* %sxpinfo33, align 4
  %380 = and i64 %379, 31
  %381 = trunc i64 %380 to i32
  %382 = icmp eq i32 %381, 13
  %383 = and i1 %378, %382
  br i1 %383, label %410, label %399

BB18:                                             ; preds = %371
  %384 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %385 = getelementptr %R_bcstack_t, %R_bcstack_t* %384, i32 4
  store %R_bcstack_t* %385, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %386 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.1", align 8
  %387 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%15.1", align 8
  %388 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %389 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %390 = getelementptr %R_bcstack_t, %R_bcstack_t* %389, i64 -4
  %391 = bitcast %R_bcstack_t* %390 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %391, i8 0, i64 64, i1 false)
  %392 = getelementptr %R_bcstack_t, %R_bcstack_t* %389, i64 -4, i32 2
  store %struct.SEXPREC* %386, %struct.SEXPREC** %392, align 8
  %393 = getelementptr %R_bcstack_t, %R_bcstack_t* %389, i64 -3, i32 2
  store %struct.SEXPREC* @gcb_318, %struct.SEXPREC** %393, align 8
  %394 = getelementptr %R_bcstack_t, %R_bcstack_t* %389, i64 -2, i32 2
  store %struct.SEXPREC* %387, %struct.SEXPREC** %394, align 8
  %395 = getelementptr %R_bcstack_t, %R_bcstack_t* %389, i64 -1, i32 2
  store %struct.SEXPREC* %388, %struct.SEXPREC** %395, align 8
  %396 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%15.1", align 8
  call void @deopt(i8* %code, %struct.SEXPREC* %closure, i8* @epe_7966_2_25777260047, %R_bcstack_t* %args, %DeoptReason* @copool_18, %struct.SEXPREC* %396)
  %397 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %398 = getelementptr %R_bcstack_t, %R_bcstack_t* %397, i32 -4
  store %R_bcstack_t* %398, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  unreachable

399:                                              ; preds = %BB17
  %400 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %401 = getelementptr %R_bcstack_t, %R_bcstack_t* %400, i32 1
  store %R_bcstack_t* %401, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %402 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%17.0", align 8
  %403 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %404 = getelementptr %R_bcstack_t, %R_bcstack_t* %403, i64 -1
  %405 = bitcast %R_bcstack_t* %404 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %405, i8 0, i64 16, i1 false)
  %406 = getelementptr %R_bcstack_t, %R_bcstack_t* %403, i64 -1, i32 2
  store %struct.SEXPREC* %402, %struct.SEXPREC** %406, align 8
  %407 = call %struct.SEXPREC* @callBuiltin(i8* %code, i32 11, %struct.SEXPREC* @gcb_318, %struct.SEXPREC* @dcs_101, i64 1)
  %408 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %409 = getelementptr %R_bcstack_t, %R_bcstack_t* %408, i32 -1
  store %R_bcstack_t* %409, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  br label %410

410:                                              ; preds = %399, %BB17
  %"PIR%17.134" = phi %struct.SEXPREC* [ %375, %BB17 ], [ %407, %399 ]
  store %struct.SEXPREC* %"PIR%17.134", %struct.SEXPREC** %"PIR%17.1", align 8
  %411 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 1
  %412 = load %struct.SEXPREC*, %struct.SEXPREC** %411, align 8
  %413 = ptrtoint %struct.SEXPREC* %412 to i64
  %414 = icmp ule i64 %413, 2
  br i1 %414, label %423, label %415, !prof !0

415:                                              ; preds = %410
  %416 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %412, i32 0, i32 4, i32 0
  %417 = load %struct.SEXPREC*, %struct.SEXPREC** %416, align 8
  %418 = icmp eq %struct.SEXPREC* %417, @dcs_106
  br i1 %418, label %423, label %419, !prof !0

419:                                              ; preds = %415
  %sxpinfo35 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %417, i32 0, i32 0, i32 0
  %420 = load i64, i64* %sxpinfo35, align 4
  %421 = and i64 %420, 281470681743360
  %422 = icmp eq i64 %421, 0
  br i1 %422, label %notNamed36, label %429

423:                                              ; preds = %415, %410
  %424 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %425 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_as.integer, %struct.SEXPREC* %424, %struct.SEXPREC** %411)
  br label %426

426:                                              ; preds = %429, %423
  %as.integer37 = phi %struct.SEXPREC* [ %417, %429 ], [ %425, %423 ]
  %427 = icmp eq %struct.SEXPREC* %as.integer37, @dcs_107
  br i1 %427, label %432, label %430, !prof !1

notNamed36:                                       ; preds = %419
  %428 = or i64 %420, 4294967296
  store i64 %428, i64* %sxpinfo35, align 4
  br label %429

429:                                              ; preds = %notNamed36, %419
  br label %426

430:                                              ; preds = %432, %426
  %431 = icmp eq %struct.SEXPREC* %as.integer37, @dcs_106
  br i1 %431, label %439, label %433, !prof !1

432:                                              ; preds = %426
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @10, i32 0, i32 0))
  br label %430

433:                                              ; preds = %439, %430
  store %struct.SEXPREC* %as.integer37, %struct.SEXPREC** %"PIR%17.2", align 8
  %434 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%17.2", align 8
  %sxpinfo39 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %434, i32 0, i32 0, i32 0
  %435 = load i64, i64* %sxpinfo39, align 4
  %436 = and i64 %435, 31
  %437 = trunc i64 %436 to i32
  %438 = icmp eq i32 %437, 5
  br i1 %438, label %isProm38, label %442, !prof !0

439:                                              ; preds = %430
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @11, i32 0, i32 0))
  br label %433

isProm38:                                         ; preds = %433
  %440 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %434, i32 0, i32 4, i32 0
  %441 = load %struct.SEXPREC*, %struct.SEXPREC** %440, align 8
  br label %443

442:                                              ; preds = %433
  br label %443

443:                                              ; preds = %442, %isProm38
  %444 = phi %struct.SEXPREC* [ %441, %isProm38 ], [ %434, %442 ]
  %445 = icmp eq %struct.SEXPREC* @gcb_318, %444
  %446 = icmp ne %struct.SEXPREC* %444, @dcs_106
  %447 = and i1 %445, %446
  br i1 %447, label %462, label %448

448:                                              ; preds = %443
  %449 = load i64, i64* getelementptr inbounds (%struct.SEXPREC, %struct.SEXPREC* @gcb_318, i32 0, i32 0, i32 0), align 4
  %450 = and i64 %449, 31
  %451 = trunc i64 %450 to i32
  %452 = icmp eq i32 %451, 3
  %sxpinfo40 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %444, i32 0, i32 0, i32 0
  %453 = load i64, i64* %sxpinfo40, align 4
  %454 = and i64 %453, 31
  %455 = trunc i64 %454 to i32
  %456 = icmp eq i32 %455, 3
  %457 = and i1 %452, %456
  br i1 %457, label %458, label %460

458:                                              ; preds = %448
  %459 = call i1 @cksEq(%struct.SEXPREC* @gcb_318, %struct.SEXPREC* %444)
  br label %460

460:                                              ; preds = %458, %448
  %461 = phi i1 [ %459, %458 ], [ false, %448 ]
  br label %462

462:                                              ; preds = %460, %443
  %463 = phi i1 [ true, %443 ], [ %461, %460 ]
  %"PIR%17.3" = zext i1 %463 to i32
  %464 = icmp ne i32 %"PIR%17.3", 0
  br i1 %464, label %BB19, label %BB20, !prof !2

BB19:                                             ; preds = %462
  %465 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %2, i64 4
  %466 = load %struct.SEXPREC*, %struct.SEXPREC** %465, align 8
  %467 = ptrtoint %struct.SEXPREC* %466 to i64
  %468 = icmp ule i64 %467, 2
  br i1 %468, label %492, label %484, !prof !0

BB20:                                             ; preds = %462
  %469 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %470 = getelementptr %R_bcstack_t, %R_bcstack_t* %469, i32 3
  store %R_bcstack_t* %470, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %471 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.1", align 8
  %472 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%17.1", align 8
  %473 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %474 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %475 = getelementptr %R_bcstack_t, %R_bcstack_t* %474, i64 -3
  %476 = bitcast %R_bcstack_t* %475 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %476, i8 0, i64 48, i1 false)
  %477 = getelementptr %R_bcstack_t, %R_bcstack_t* %474, i64 -3, i32 2
  store %struct.SEXPREC* %471, %struct.SEXPREC** %477, align 8
  %478 = getelementptr %R_bcstack_t, %R_bcstack_t* %474, i64 -2, i32 2
  store %struct.SEXPREC* %472, %struct.SEXPREC** %478, align 8
  %479 = getelementptr %R_bcstack_t, %R_bcstack_t* %474, i64 -1, i32 2
  store %struct.SEXPREC* %473, %struct.SEXPREC** %479, align 8
  %480 = icmp ne i32 %"PIR%17.3", 0
  %481 = select i1 %480, %struct.SEXPREC* @dcs_103, %struct.SEXPREC* @dcs_105
  call void @deopt(i8* %code, %struct.SEXPREC* %closure, i8* @epe_7966_3_25777260047, %R_bcstack_t* %args, %DeoptReason* @copool_19, %struct.SEXPREC* %481)
  %482 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %483 = getelementptr %R_bcstack_t, %R_bcstack_t* %482, i32 -3
  store %R_bcstack_t* %483, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  unreachable

484:                                              ; preds = %BB19
  %485 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %466, i32 0, i32 4, i32 0
  %486 = load %struct.SEXPREC*, %struct.SEXPREC** %485, align 8
  %487 = icmp eq %struct.SEXPREC* %486, @dcs_106
  br i1 %487, label %492, label %488, !prof !0

488:                                              ; preds = %484
  %sxpinfo41 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %486, i32 0, i32 0, i32 0
  %489 = load i64, i64* %sxpinfo41, align 4
  %490 = and i64 %489, 281470681743360
  %491 = icmp eq i64 %490, 0
  br i1 %491, label %notNamed42, label %498

492:                                              ; preds = %484, %BB19
  %493 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %494 = call %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC* @sym_stop, %struct.SEXPREC* %493, %struct.SEXPREC** %465)
  br label %495

495:                                              ; preds = %498, %492
  %stop = phi %struct.SEXPREC* [ %486, %498 ], [ %494, %492 ]
  %496 = icmp eq %struct.SEXPREC* %stop, @dcs_107
  br i1 %496, label %501, label %499, !prof !1

notNamed42:                                       ; preds = %488
  %497 = or i64 %489, 4294967296
  store i64 %497, i64* %sxpinfo41, align 4
  br label %498

498:                                              ; preds = %notNamed42, %488
  br label %495

499:                                              ; preds = %501, %495
  %500 = icmp eq %struct.SEXPREC* %stop, @dcs_106
  br i1 %500, label %508, label %502, !prof !1

501:                                              ; preds = %495
  call void @error(i8* getelementptr inbounds ([37 x i8], [37 x i8]* @12, i32 0, i32 0))
  br label %499

502:                                              ; preds = %508, %499
  store %struct.SEXPREC* %stop, %struct.SEXPREC** %"PIR%19.0", align 8
  %503 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%19.0", align 8
  %sxpinfo43 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %503, i32 0, i32 0, i32 0
  %504 = load i64, i64* %sxpinfo43, align 4
  %505 = and i64 %504, 31
  %506 = trunc i64 %505 to i32
  %507 = icmp eq i32 %506, 5
  br i1 %507, label %509, label %515

508:                                              ; preds = %499
  call void @error(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @13, i32 0, i32 0))
  br label %502

509:                                              ; preds = %502
  %510 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %503, i32 0, i32 4, i32 0
  %511 = load %struct.SEXPREC*, %struct.SEXPREC** %510, align 8
  %512 = icmp eq %struct.SEXPREC* %511, @dcs_106
  br i1 %512, label %513, label %516, !prof !0

513:                                              ; preds = %509
  %514 = call %struct.SEXPREC* @forcePromise(%struct.SEXPREC* %503)
  br label %517

515:                                              ; preds = %502
  br label %517

516:                                              ; preds = %509
  br label %517

517:                                              ; preds = %516, %515, %513
  %"PIR%19.144" = phi %struct.SEXPREC* [ %514, %513 ], [ %503, %515 ], [ %511, %516 ]
  store %struct.SEXPREC* %"PIR%19.144", %struct.SEXPREC** %"PIR%19.1", align 8
  %518 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%19.1", align 8
  %519 = icmp ne %struct.SEXPREC* %518, @dcs_106
  %520 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %518, i32 0, i32 1
  %521 = load %struct.SEXPREC*, %struct.SEXPREC** %520, align 8
  %522 = icmp eq %struct.SEXPREC* %521, @dcs_104
  %523 = and i1 %519, %522
  %524 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %518, i32 0, i32 1
  %525 = load %struct.SEXPREC*, %struct.SEXPREC** %524, align 8
  %526 = icmp eq %struct.SEXPREC* %525, @dcs_104
  %sxpinfo45 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %518, i32 0, i32 0, i32 0
  %527 = load i64, i64* %sxpinfo45, align 4
  %528 = and i64 %527, 64
  %529 = icmp ne i64 0, %528
  %530 = xor i1 %529, true
  %531 = and i1 %530, %526
  br i1 %531, label %540, label %532

532:                                              ; preds = %517
  %533 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %525, i32 0, i32 4, i32 2
  %534 = load %struct.SEXPREC*, %struct.SEXPREC** %533, align 8
  %535 = icmp eq %struct.SEXPREC* %534, @dcs_111
  %536 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %525, i32 0, i32 4, i32 1
  %537 = load %struct.SEXPREC*, %struct.SEXPREC** %536, align 8
  %538 = icmp eq %struct.SEXPREC* %537, @dcs_104
  %539 = and i1 %535, %538
  br label %540

540:                                              ; preds = %532, %517
  %541 = phi i1 [ true, %517 ], [ %539, %532 ]
  %542 = and i1 %523, %541
  %"PIR%19.2" = zext i1 %542 to i32
  %543 = icmp ne i32 %"PIR%19.2", 0
  br i1 %543, label %BB21, label %BB22, !prof !2

BB21:                                             ; preds = %540
  %"PIR%21.046" = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%19.1", align 8
  store %struct.SEXPREC* %"PIR%21.046", %struct.SEXPREC** %"PIR%21.0", align 8
  %544 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%21.0", align 8
  %545 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %544, i32 0, i32 1
  %546 = load %struct.SEXPREC*, %struct.SEXPREC** %545, align 8
  %547 = icmp eq %struct.SEXPREC* %546, @dcs_104
  %sxpinfo47 = getelementptr %struct.SEXPREC, %struct.SEXPREC* %544, i32 0, i32 0, i32 0
  %548 = load i64, i64* %sxpinfo47, align 4
  %549 = and i64 %548, 31
  %550 = trunc i64 %549 to i32
  %551 = icmp eq i32 %550, 13
  %552 = and i1 %547, %551
  br i1 %552, label %581, label %570

BB22:                                             ; preds = %540
  %553 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %554 = getelementptr %R_bcstack_t, %R_bcstack_t* %553, i32 5
  store %R_bcstack_t* %554, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %555 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.1", align 8
  %556 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%17.1", align 8
  %557 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%19.1", align 8
  %558 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %559 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %560 = getelementptr %R_bcstack_t, %R_bcstack_t* %559, i64 -5
  %561 = bitcast %R_bcstack_t* %560 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %561, i8 0, i64 80, i1 false)
  %562 = getelementptr %R_bcstack_t, %R_bcstack_t* %559, i64 -5, i32 2
  store %struct.SEXPREC* %555, %struct.SEXPREC** %562, align 8
  %563 = getelementptr %R_bcstack_t, %R_bcstack_t* %559, i64 -4, i32 2
  store %struct.SEXPREC* %556, %struct.SEXPREC** %563, align 8
  %564 = getelementptr %R_bcstack_t, %R_bcstack_t* %559, i64 -3, i32 2
  store %struct.SEXPREC* @gcb_318, %struct.SEXPREC** %564, align 8
  %565 = getelementptr %R_bcstack_t, %R_bcstack_t* %559, i64 -2, i32 2
  store %struct.SEXPREC* %557, %struct.SEXPREC** %565, align 8
  %566 = getelementptr %R_bcstack_t, %R_bcstack_t* %559, i64 -1, i32 2
  store %struct.SEXPREC* %558, %struct.SEXPREC** %566, align 8
  %567 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%19.1", align 8
  call void @deopt(i8* %code, %struct.SEXPREC* %closure, i8* @epe_7966_4_25777260047, %R_bcstack_t* %args, %DeoptReason* @copool_20, %struct.SEXPREC* %567)
  %568 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %569 = getelementptr %R_bcstack_t, %R_bcstack_t* %568, i32 -5
  store %R_bcstack_t* %569, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  unreachable

570:                                              ; preds = %BB21
  %571 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %572 = getelementptr %R_bcstack_t, %R_bcstack_t* %571, i32 1
  store %R_bcstack_t* %572, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %573 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%21.0", align 8
  %574 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %575 = getelementptr %R_bcstack_t, %R_bcstack_t* %574, i64 -1
  %576 = bitcast %R_bcstack_t* %575 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %576, i8 0, i64 16, i1 false)
  %577 = getelementptr %R_bcstack_t, %R_bcstack_t* %574, i64 -1, i32 2
  store %struct.SEXPREC* %573, %struct.SEXPREC** %577, align 8
  %578 = call %struct.SEXPREC* @callBuiltin(i8* %code, i32 13, %struct.SEXPREC* @gcb_318, %struct.SEXPREC* @dcs_101, i64 1)
  %579 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %580 = getelementptr %R_bcstack_t, %R_bcstack_t* %579, i32 -1
  store %R_bcstack_t* %580, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  br label %581

581:                                              ; preds = %570, %BB21
  %"PIR%21.148" = phi %struct.SEXPREC* [ %544, %BB21 ], [ %578, %570 ]
  store %struct.SEXPREC* %"PIR%21.148", %struct.SEXPREC** %"PIR%21.1", align 8
  %582 = load %struct.SEXPREC*, %struct.SEXPREC** %PIRe0.5, align 8
  %583 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%3.1", align 8
  %584 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%17.1", align 8
  %585 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%21.1", align 8
  %586 = call %struct.SEXPREC* @consNr(%struct.SEXPREC* %585, %struct.SEXPREC* @dcs_104)
  %587 = call %struct.SEXPREC* @consNr(%struct.SEXPREC* %584, %struct.SEXPREC* %586)
  %588 = call %struct.SEXPREC* @consNr(%struct.SEXPREC* %583, %struct.SEXPREC* %587)
  %589 = getelementptr %R_bcstack_t, %R_bcstack_t* %1, i64 4, i32 2
  store volatile %struct.SEXPREC* %588, %struct.SEXPREC** %589, align 8
  %590 = load i32, i32* @copool_15, align 4
  %591 = load %struct.SEXPREC*, %struct.SEXPREC** getelementptr (%struct.SEXPREC*, %struct.SEXPREC** inttoptr (i64* @spe_constantPool to %struct.SEXPREC**), i32 1), align 8
  %592 = bitcast %struct.SEXPREC* %591 to %struct.VECTOR_SEXPREC*
  %593 = getelementptr %struct.VECTOR_SEXPREC, %struct.VECTOR_SEXPREC* %592, i32 1
  %594 = bitcast %struct.VECTOR_SEXPREC* %593 to %struct.SEXPREC**
  %595 = getelementptr %struct.SEXPREC*, %struct.SEXPREC** %594, i32 %590
  %596 = load %struct.SEXPREC*, %struct.SEXPREC** %595, align 8
  store i32 1, i32* @spe_Visible, align 4
  %"PIR%21.249" = call %struct.SEXPREC* @cod_340(%struct.SEXPREC* %596, %struct.SEXPREC* @gcb_340, %struct.SEXPREC* %588, %struct.SEXPREC* %582)
  store i32 1, i32* @spe_Visible, align 4
  store %struct.SEXPREC* %"PIR%21.249", %struct.SEXPREC** %"PIR%21.2", align 8
  %597 = load %struct.SEXPREC*, %struct.SEXPREC** %"PIR%21.2", align 8
  %598 = load %R_bcstack_t*, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  %599 = getelementptr %R_bcstack_t, %R_bcstack_t* %598, i32 -5
  store %R_bcstack_t* %599, %R_bcstack_t** @spe_BCNodeStackTop, align 8
  ret %struct.SEXPREC* %597
}

declare %struct.SEXPREC* @forcePromise(%struct.SEXPREC*)

declare %struct.SEXPREC* @createBindingCellImpl(%struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*)

declare %struct.SEXPREC* @createEnvironment(%struct.SEXPREC*, %struct.SEXPREC*, i32)

declare %struct.SEXPREC* @ldvarCacheMiss(%struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC**)

declare void @error(i8*)

declare i1 @cksEq(%struct.SEXPREC*, %struct.SEXPREC*)

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #0

declare %struct.SEXPREC* @callBuiltin(i8*, i32, %struct.SEXPREC*, %struct.SEXPREC*, i64)

declare void @setCar(%struct.SEXPREC*, %struct.SEXPREC*)

declare void @stvar(%struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*)

declare %struct.SEXPREC* @cod_340(%struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*, %struct.SEXPREC*)

declare %struct.SEXPREC* @consNr(%struct.SEXPREC*, %struct.SEXPREC*)

declare %struct.SEXPREC* @ldfun(%struct.SEXPREC*, %struct.SEXPREC*)

declare void @deopt(i8*, %struct.SEXPREC*, i8*, %R_bcstack_t*, %DeoptReason*, %struct.SEXPREC*)

attributes #0 = { argmemonly nounwind willreturn }

!0 = !{!"branch_weights", i32 1, i32 1000}
!1 = !{!"branch_weights", i32 1, i32 100000000}
!2 = !{!"branch_weights", i32 100000000, i32 1}
