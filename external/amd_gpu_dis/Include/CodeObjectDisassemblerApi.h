/*=-- CodeObjectDisassemblerApi.hpp - Disassembler for AMD GPU Code Object ----------- C++ --=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------------------------===//
//
/// \file
///
/// This file contains declaration for AMD GPU Code Object Dissassembler API
//
//===--------------------------------------------------------------------------------------===*/
#ifndef CODE_OBJECT_DISASSEMBLER_API_H_
#define CODE_OBJECT_DISASSEMBLER_API_H_

#include <stdint.h> /* for uintXX_t type */

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#if AMD_GPU_DIS_NO_EXPORTS
#define AMD_GPU_DIS_API_ENTRY /* empty */
#elif defined(AMD_GPU_DIS_EXPORTS)
/* Export symbols when building the library on Windows. */
#define AMD_GPU_DIS_API_ENTRY __declspec(dllexport)
#else
/* No symbol export when used by the library user on Windows. */
#define AMD_GPU_DIS_API_ENTRY __declspec(dllimport)
#endif
/* The API calling convention on Windows. */
#define AMD_GPU_DIS_API_CALL __cdecl
#else
#if __GNUC__ >= 4
/* Use symbol visibility control supported by GCC 4.x and newer. */
#define AMD_GPU_DIS_API_ENTRY __attribute__((visibility("default")))
#else
/* No symbol visibility control for GCC older than 4.0. */
#define AMD_GPU_DIS_API_ENTRY
#endif
/* The API calling convention on Linux. */
#define AMD_GPU_DIS_API_CALL
#endif

#define AMD_GPU_DIS_MAJOR_VERSION_NUMBER 1
#define AMD_GPU_DIS_MINOR_VERSION_NUMBER (sizeof(AmdGpuDisApiTable))

#define AMD_GPU_DIS_INVALID_ADDRESS UINT64_MAX

typedef enum {
  AMD_GPU_DIS_STATUS_SUCCESS = 0,
  AMD_GPU_DIS_STATUS_FAILED = -1,
  AMD_GPU_DIS_STATUS_NULL_POINTER = -2,
  AMD_GPU_DIS_STATUS_MEMORY_ALLOCATION_FAILURE = -3,
  AMD_GPU_DIS_STATUS_INVALID_INPUT = -4,
  AMD_GPU_DIS_STATUS_INVALID_CONTEXT_HANDLE = -5,
  AMD_GPU_DIS_STATUS_INVALID_CALLBACK = -6,
  AMD_GPU_DIS_STATUS_INVALID_CFG_BLOCK = -7,
  AMD_GPU_DIS_STATUS_INVALID_PC = -8,
  AMD_GPU_DIS_STATUS_OUT_OF_RANGE = -9
} AmdGpuDisStatus;

typedef struct {
  uint64_t handle;
} AmdGpuDisContext;

/*
/// @brief This will be the entry API to get all the function pointers to be
/// called
*/
extern AMD_GPU_DIS_API_ENTRY AmdGpuDisStatus AMD_GPU_DIS_API_CALL
AmdGpuDisGetApiTable(void *ApiTableOut);

/*
/// @brief Create context handle
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisCreateContext_fn)(
    AmdGpuDisContext *Context);

/*
/// @brief Destroy context handle
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisDestroyContext_fn)(
    AmdGpuDisContext Context);

/*
/// @brief Load code object from buffer
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisLoadCodeObjectBuffer_fn)(
    AmdGpuDisContext Context, const char *CodeObjectBuffer,
    size_t CodeObjectBufferSize, bool EmitCFG);

/*
/// @brief Query disassembly string size
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetDisassemblyStringSize_fn)(
    AmdGpuDisContext Context, size_t *DisassemblyStringSize);

/*
/// @brief Query disassembly string
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisGetDisassemblyString_fn)(
    AmdGpuDisContext Context, char *DisassemblyString);

/*
/// @brief Query CFG head labels
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgHeads_fn)(
    AmdGpuDisContext Context,
    AmdGpuDisStatus (*Callback)(const char *head, void *UserData),
    void *UserData);

/*
/// @brief Query the destination block labels for a given block of CFG
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgBasicBlockDestinations_fn)(
    AmdGpuDisContext Context, const char *Block,
    AmdGpuDisStatus (*Callback)(const char *DstBlock, bool IsBranchTarget,
                                void *UserData),
    void *UserData);

/*
/// @brief Query all instructions for a given block of CFG
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgBasicBlockInstructions_fn)(
    AmdGpuDisContext Context, const char *Block,
    AmdGpuDisStatus (*Callback)(const char *Inst, const char *Comment,
                                void *UserData),
    void *UserData);

/*
/// @brief Query flattened block labels for a given head block of CFG
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgFlatBasicBlocks_fn)(
    AmdGpuDisContext Context, const char *HeadBlock,
    AmdGpuDisStatus (*Callback)(const char *Block, void *UserData),
    void *UserData);

/*
/// @brief Query an instruction for a given block of CFG with the index relative
/// to the start of the block
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetCfgBasicBlockInstructionByIndex_fn)(
    AmdGpuDisContext Context, const char *Block, size_t Index,
    AmdGpuDisStatus (*Callback)(const char *Inst, const char *Comment,
                                void *UserData),
    void *UserData);

/*
/// @brief Query the program counter for a given block of CFG with the index relative
/// to the start of the block
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetProgramCounterByIndex_fn)(
    AmdGpuDisContext Context, const char *Block, size_t Index,
    uint64_t *ProgramCounter);

/*
/// @brief Query the address of an instruction by the address of its
/// block and index within that block.
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL
        *AmdGpuDisGetInstructionAddressByBlockAddressAndIndex_fn)(
    AmdGpuDisContext Context, uint64_t BlockAddr, uint64_t Index,
    uint64_t *InstrAddr);

/*
/// @brief Query the location of the instruction with the program counter
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL
        *AmdGpuDisGetCfgInstructionLocationByProgramCounter_fn)(
    AmdGpuDisContext Context, uint64_t ProgramCounter,
    uint64_t *BasicBlockAddress, uint64_t *Offset);

/*
/// @brief Query the max program counter value
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetMaxProgramCounter_fn)(
    AmdGpuDisContext Context, uint64_t *ProgramCounter);

/*
/// @brief Query the starting program counter value of the first instruction
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetInstructionStartingProgramCounter_fn)(
    AmdGpuDisContext Context, uint64_t *ProgramCounter);

/*
/// @brief Query the number of instructions for a given block of CFG
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisGetCfgBasicBlockSize_fn)(
    AmdGpuDisContext Context, const char *Block, size_t *BlockSize);

/*
/// @brief Query the result if unknown instructions were seen for current CFG
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisIfSeenCfgUnknownInstructions_fn)(
    AmdGpuDisContext Context, const char *HeadBlock, bool *SeenUnknownInst);

/*
/// @brief Query the basic block name string by address
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetBasicBlockNameByAddress_fn)(
    AmdGpuDisContext Context, uint64_t BasicBlockAddress,
    AmdGpuDisStatus (*Callback)(const char *BasicBlockName, void *UserData),
    void *UserData);

/*
/// @brief Query the basic block address by name string
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetBasicBlockAddressByName_fn)(
    AmdGpuDisContext Context, const char *BasicBlockName,
    uint64_t *BasicBlockAddress);

/*
/// @brief Query the destination block addresses for a given block address of CFG
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgBasicBlockDestinationsByAddress_fn)(
    AmdGpuDisContext Context, uint64_t BasicBlockAddress,
    AmdGpuDisStatus (*Callback)(uint64_t DstBasicBlockAddress,
                                bool IsBranchTarget, void *UserData),
    void *UserData);

/*
/// @brief Query an instruction for a given block address of CFG with the offset of the block
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetCfgBasicBlockInstructionLine_fn)(
    AmdGpuDisContext Context, uint64_t BasicBlockAddress, uint64_t Offset,
    AmdGpuDisStatus (*Callback)(const char *InstStr, const char *CommentStr,
                                void *UserData),
    void *UserData);

/*
/// @brief Query the number of instructions for a given block of CFG by address
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetCfgBasicBlockSizeByAddress_fn)(
    AmdGpuDisContext Context, uint64_t BasicBlockAddress,
    uint64_t *BasicBlockSize);

/*
/// @brief Query the location address of the instruction with the program counter
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisGetCfgInstructionAddressByProgramCounter_fn)(
    AmdGpuDisContext Context, uint64_t ProgramCounter,
    uint64_t *BasicBlockAddress, uint64_t *Offset);

/*
/// @brief Query flattened block addresses for a given head block address of CFG
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgFlatBasicBlockAddresses_fn)(
    AmdGpuDisContext Context, uint64_t HeadBlockAddress,
    AmdGpuDisStatus (*Callback)(uint64_t Address, void *UserData),
    void *UserData);

/*
/// @brief Query CFG head addresses
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL *AmdGpuDisIterateCfgHeadAddresses_fn)(
    AmdGpuDisContext Context,
    AmdGpuDisStatus (*Callback)(uint64_t Address, void *UserData),
    void *UserData);

/*
/// @brief Query all instructions for a given block address of CFG
*/
typedef AmdGpuDisStatus(
    AMD_GPU_DIS_API_CALL
        *AmdGpuDisIterateCfgBasicBlockInstructionsByAddress_fn)(
    AmdGpuDisContext Context, uint64_t BasicBlockAddress,
    AmdGpuDisStatus (*Callback)(const char *InstStr, const char *CommentStr,
                                void *UserData),
    void *UserData);

/*
/// @brief Query a block by its address.
*/
typedef struct AmdGpuDisBlock AmdGpuDisBlock;
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisGetBlockByAddress_fn)(
    AmdGpuDisContext Context, uint64_t BlockAddr, const AmdGpuDisBlock **Block);

/*
/// @brief Query the address of a given block.
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisGetBlockAddress_fn)(
    const AmdGpuDisBlock *Block, uint64_t *BlockAddr);

/*
/// @brief Set a disassembling option.
///
/// @description
/// Currently supported options are:
/// - `mattr`
///   Target architecture features. `Value` is a string
///   (`const char*`) of comma-separated list of target features.
*/
typedef AmdGpuDisStatus(AMD_GPU_DIS_API_CALL *AmdGpuDisSetOption_fn)(
    AmdGpuDisContext Context, const char *Name, const void *Value);

typedef struct {
  uint32_t MajorVersion;
  uint32_t MinorVersion;

  AmdGpuDisCreateContext_fn AmdGpuDisCreateContext;
  AmdGpuDisDestroyContext_fn AmdGpuDisDestroyContext;
  AmdGpuDisLoadCodeObjectBuffer_fn AmdGpuDisLoadCodeObjectBuffer;
  AmdGpuDisGetDisassemblyStringSize_fn AmdGpuDisGetDisassemblyStringSize;
  AmdGpuDisGetDisassemblyString_fn AmdGpuDisGetDisassemblyString;
  AmdGpuDisIterateCfgHeads_fn AmdGpuDisIterateCfgHeads;
  AmdGpuDisIterateCfgBasicBlockDestinations_fn
      AmdGpuDisIterateCfgBasicBlockDestinations;
  AmdGpuDisIterateCfgBasicBlockInstructions_fn
      AmdGpuDisIterateCfgBasicBlockInstructions;
  AmdGpuDisIterateCfgFlatBasicBlocks_fn AmdGpuDisIterateCfgFlatBasicBlocks;
  AmdGpuDisGetCfgBasicBlockInstructionByIndex_fn
      AmdGpuDisGetCfgBasicBlockInstructionByIndex;
  AmdGpuDisGetProgramCounterByIndex_fn AmdGpuDisGetProgramCounterByIndex;
  AmdGpuDisGetCfgInstructionLocationByProgramCounter_fn
      AmdGpuDisGetCfgInstructionLocationByProgramCounter;
  AmdGpuDisGetCfgBasicBlockSize_fn AmdGpuDisGetCfgBasicBlockSize;
  AmdGpuDisIfSeenCfgUnknownInstructions_fn
      AmdGpuDisIfSeenCfgUnknownInstructions;
  AmdGpuDisGetMaxProgramCounter_fn AmdGpuDisGetMaxProgramCounter;
  AmdGpuDisGetInstructionStartingProgramCounter_fn
      AmdGpuDisGetInstructionStartingProgramCounter;
  AmdGpuDisGetBasicBlockNameByAddress_fn AmdGpuDisGetBasicBlockNameByAddress;
  AmdGpuDisGetBasicBlockAddressByName_fn AmdGpuDisGetBasicBlockAddressByName;
  AmdGpuDisIterateCfgBasicBlockDestinationsByAddress_fn
      AmdGpuDisIterateCfgBasicBlockDestinationsByAddress;
  AmdGpuDisGetCfgBasicBlockInstructionLine_fn
      AmdGpuDisGetCfgBasicBlockInstructionLine;
  AmdGpuDisGetCfgBasicBlockSizeByAddress_fn
      AmdGpuDisGetCfgBasicBlockSizeByAddress;
  AmdGpuDisGetCfgInstructionAddressByProgramCounter_fn
      AmdGpuDisGetCfgInstructionAddressByProgramCounter;
  AmdGpuDisIterateCfgFlatBasicBlockAddresses_fn
      AmdGpuDisIterateCfgFlatBasicBlockAddresses;
  AmdGpuDisIterateCfgHeadAddresses_fn AmdGpuDisIterateCfgHeadAddresses;
  AmdGpuDisIterateCfgBasicBlockInstructionsByAddress_fn
      AmdGpuDisIterateCfgBasicBlockInstructionsByAddress;
  AmdGpuDisGetInstructionAddressByBlockAddressAndIndex_fn
      AmdGpuDisGetInstructionAddressByBlockAddressAndIndex;
  AmdGpuDisGetBlockByAddress_fn GetBlockByAddress;
  AmdGpuDisGetBlockAddress_fn GetBlockAddress;
  AmdGpuDisSetOption_fn SetOption;
} AmdGpuDisApiTable;

#ifdef __cplusplus
}
#endif

#endif /* CODE_OBJECT_DISASSEMBLER_API_H_ */
