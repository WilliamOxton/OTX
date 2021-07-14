//
// Created by Alex on 4/3/20.
//

#include <set>
#include <map>
#include <string>
#include <sstream>
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "ATSGXIRTag.h"

//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
using namespace llvm;
using namespace std;

#define DEBUG_TYPE "ATSGX_IR_TAG"

unsigned llvm::ATSGX_MBB_isLeaky(const MachineBasicBlock *mbb){
  const MachineFunction *mf = mbb->getParent();
  const BasicBlock *obj_bb = mbb->getBasicBlock();
  //分析：MachineBB 可能没有对应的BB，也就无法找到对应的Func，
  //从而只能通过当前的MachineFunc对应的Function
  const Function *obj_func = obj_bb? obj_bb->getParent(): &mf->getFunction();

  if(ATSGX_UNSAFE == ATSGX_isLeaky<Function>(obj_func)){
    return ATSGX_UNSAFE;
  }

  unsigned state=ATSGX_SAFE;

  if(!obj_bb) return state;

  for(auto &I:*obj_bb){
    state |= ATSGX_isLeaky<Instruction>(&I);
  }
  return state;
}

bool ATSGXIRTagPass::runOnModule(Module &M){
  bool isChanged = false;
  if(testing)
    return false;

  std::set<Function *> working_set;

  parse_file(leakage_file_path+".code", file_2_path_mapping,
             func_2_file_mapping, dirty_lines, ATSGX_Debug);

  parse_file(leakage_file_path+".data", file_2_path_mapping,
             func_2_file_mapping, dirty_lines_data, ATSGX_Debug);


  SmallVector<Function *, 10> _temp_working_list;
  for (auto &F : M) {

    unsigned func_taint_state = ATSGX_SAFE;
    LLVMContext &ctx =  F.getContext();
    for (auto &B: F.getBasicBlockList()){

      //处理基本块中的每条指令
      _temp_working_list.clear();
      unsigned bb_taint_state = ATSGX_SAFE;
      for(auto &I: B.instructionsWithoutDebug()){

        //记录基本块中被调用的指令
        if(CallBase *CI = dyn_cast<CallBase>(&I)){
          Function *callee = CI->getCalledFunction();
          if(callee)
            _temp_working_list.push_back(callee);
        }

        unsigned ins_taint_state = ATSGX_SAFE;
        //get each instruction's debug information
        const DebugLoc &debugLoc = I.getDebugLoc();
        if (!debugLoc)
          continue;

        //get the dirty set
        StringRef path = debugLoc->getDirectory();
        StringRef name = debugLoc->getFilename();
        unsigned line = debugLoc.getLine();
        std::string file_path =  path.str()+ "/" + name.str();

        auto itr_code = dirty_lines.find(file_path);
        if(itr_code != dirty_lines.end()){
          std::set<unsigned > &file_dirty_lines = itr_code->second;
          if(file_dirty_lines.find(line)!=file_dirty_lines.end()){
            ins_taint_state |= ATSGX_CODE_LEAKY;
          }
        }

        auto itr_data = dirty_lines_data.find(file_path);
        if(itr_data != dirty_lines_data.end()){
          std::set<unsigned > &file_dirty_lines = itr_data->second;
          if(file_dirty_lines.find(line)!=file_dirty_lines.end()){
            ins_taint_state |= ATSGX_DATA_LEAKY;
          }
        }

        ATSGX_setLeakageState<Instruction>(ctx, &I, ins_taint_state);
        bb_taint_state |= ins_taint_state;
      }

      //将被调用的函数添加到working_list中
      if(bb_taint_state & ATSGX_CODE_LEAKY){
        while(!_temp_working_list.empty()){
          Function * _func= _temp_working_list.pop_back_val();
          working_set.insert(_func);
        }
      }
      func_taint_state |= bb_taint_state;
    }

    //if this function has leakages, we should mark it as tainted as well.
    ATSGX_setLeakageState<Function>(ctx, &F, func_taint_state);

    isChanged = isChanged | func_taint_state;
  }

  //将敏感基本块内被调用的函数标记为 ATSGX_UNSAFE
  //并且递推标记
  std::set<Function*> unsafe_func_set;
  while(!working_set.empty()){
    Function *func = *working_set.begin();
    working_set.erase(func);
    unsafe_func_set.insert(func);

    LLVMContext &ctx =  func->getContext();
    if(func->hasMetadata(ATSGX_CODE_STATE) &&
        (ATSGX_isLeaky<Function>(func) & ATSGX_CODE_DATA_LEAKY)){
      continue;
    }

    ATSGX_setLeakageState<Function>(ctx, func, ATSGX_UNSAFE);

    for(auto &B: func->getBasicBlockList())
      for(auto &I : B.instructionsWithoutDebug()){
        if(CallBase *CI = dyn_cast<CallBase>(&I)){
          Function *callee = CI->getCalledFunction();
          //如果callee已经存在unsafe_func_set的话，就没必要再次添加了
          if(callee && unsafe_func_set.find(callee)==unsafe_func_set.end()){
              working_set.insert(callee);
          }
        }
      }
  }

  if(ATSGX_Debug){
    debug_check_taint_state(M);
    debug_show_unsafe_functions(unsafe_func_set);
  }

  return isChanged;
}

// We don't modify the program, so we preserve all analyses.
void ATSGXIRTagPass::getAnalysisUsage(AnalysisUsage &AU) const{
  AU.setPreservesAll();
}


char ATSGXIRTagPass::ID = 0;
INITIALIZE_PASS(ATSGXIRTagPass, "ATSGX-IR-Tag", "ATSGX IR Tag", false,
                      true)

ModulePass *llvm::createATSGXIRTagPass(const StringRef &leakage_file_path, bool ATSGXDebug, int ATSGXOpt) {
  return new ATSGXIRTagPass(leakage_file_path, ATSGXOpt, ATSGXDebug);
}
