// Print the hast and src's associated with its index
#define PRINT_POPULATED_SRC_DATA 0

// Serializer Debugging
// Prints the progress of the serializer
#define PRINT_SERIALIZER_PROGRESS 1
// Print contant pool entries
#define PRINT_CP_ENTRIES 0
// Print src pool entries
#define PRINT_SRC_ENTRIES 0
// Print promise src entries
#define PRINT_PROM_ENTRIES 0
// Prints the module before updating the constant pool entries
#define PRINT_MODULE_BEFORE_POOL_PATCHES 0
// Prints the module after updating the constant pool entries
#define PRINT_MODULE_AFTER_POOL_PATCHES 0

// Compilation Debugging
// Prints the LLVM module before making any name changes
#define BACKEND_PRINT_INITIAL_LLVM 0
// Prints the LLVM module after making the name changes
#define BACKEND_PRINT_FINAL_LLVM 0
// Prints the old name in the module and the patched name
#define BACKEND_PRINT_NAME_UPDATES 0
// Prints the promise map
#define PRINT_PROM_MAP 0
// Prints the done map
#define PRINT_DONE_MAP 0


// Toggle Lowering Features
#define USE_BINDING_CACHE_WHILE_LOWERING 1


// Rename globalConstants - Adds a prefix to the CP lookup indices, so we can look them up during serialization
//
// Prefix: "copool_"
// 1. constants
// 2. DeoptReason
// 3. namesStore - for passing CP indicies to createStubEnvironment,  dotsCall, namedCall
//
#define PATCH_GLOBAL_CONSTANT_NAMES 1

// R special symbol patches
//
// Prefix: "spe_"
// Symbol: "spe_Visible"
#define PATCH_SET_VISIBLE 1
// Symbol: "spe_BCNodeStackTop"
#define PATCH_NODE_STACK_TOP 1
// Symbol: "spe_constantPool"
#define PATCH_CONSTANT_POOL_PTR 1
// Symbol: "spe_returnedValue"
#define PATCH_RETURNED_VALUE 1

// Instruction assert fail message, checktype message
// Prefix: "msg_"
#define PATCH_MSG 1


// Direct Constant Symbols
// Prefix: "dcs_"
//
// 100 - R_GlobalEnv
// 101 - R_BaseEnv
// 102 - R_BaseNamespace
// 103 - R_TrueValue
// 104 - R_NilValue
// 105 - R_FalseValue
// 106 - R_UnboundValue
// 107 - R_MissingArg
// 108 - R_LogicalNAValue
// 109 - R_EmptyEnv
// 110 - R_RestartToken
// 111 - R_DimSymbol
//
#define PATCH_100_102 1
#define PATCH_103_109 1
#define PATCH_110_111 1

// Patch SYMSXP
// Prefix: "sym_"
#define PATCH_SYMSXP 1

// Patch BUILTINSXP
#define PATCH_BUILTINSXP 1

// TODO : patch SPECIALSXP, only functions handled currently
#define PATCH_SPECIALSXP 1

// Patch Constant Pool Entries, needs PATCH_GLOBAL_CONSTANT_NAMES to work
#define PATCH_CP_ENTRIES 1

// Patch Constant Pool Entries, needs PATCH_GLOBAL_CONSTANT_NAMES to work
#define PATCH_SRCIDX_ENTRY 1

// Patch callRBuiltin
#define PATCH_BUILTINCALL 1

// Patch opaqueTrue
#define PATCH_OPAQUE_TRUE 1

// Change calculation of PC from start of BC instructions, NEEDED FOR TRY_PATCH_DEOPTREASON and TRY_PATCH_DEOPTMETADATA
#define TRY_PATCH_DEOPTREASON_PC 0

// Try patching DeoptReason
#define TRY_PATCH_DEOPTREASON 0

// Try patching DeoptMetadata
#define TRY_PATCH_DEOPTMETADATA 0

// Try patching DeoptMetadata
#define TRY_PATCH_STATIC_CALL3 0

#define TRY_PATCH_OPT_DISPATCH 0




// Print the compiled context for each hast
#define DEBUG_PRINT_COMPILED_CONTEXT 0

// Show error info if something goes wrong
#define DEBUG_ERR_MSG 1

#define DEBUG_INSTRUMENT_RIR_CALL 0

#define DEBUG_PRINT_LOAD_FUN 0

#define DEBUG_PRINT_LdVar_Inst 0
#define DEBUG_PRINT_Identical_Inst 0
#define DEBUG_PRINT_MkEnv_Inst 0
#define DEBUG_PRINT_Deopt_Inst 0
#define DEBUG_PRINT_DeoptReason_Load 0


// Add debug locations in the llvm code
#define ADD_EXTRA_DEBUGGING_DATA 0
#define DEBUG_LOCATIONS 0

#define DEBUG_NATIVE_LOCATIONS 0
