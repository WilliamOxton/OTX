//
// Created by Alex on 4/3/20.
//

#ifndef LLVM_ATSGXIRTAG_H
#define LLVM_ATSGXIRTAG_H
#include <set>
#include <map>
#include <string>
#include <sstream>
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/CodeGen/MachineBasicBlock.h"

#define ATSGX_CODE_STATE "atsgx_code_state"
#define ATSGX_DATA_STATE "atsgx_data_state"

#define ATSGX_LEAKAGE_STR "leakage"
#define ATSGX_UNSAFE_STR  "unsafe"
#define ATSGX_SAFE_STR    "safe"

//N->S
#define NtoS_None   "atsgx.n2s.none" //S不用rax为src
#define NtoS_SRC    "atsgx.n2s.src" //S使用rax为src
//S1->S2
#define StoS_DEST_None  "atsgx.s2s.dest.none" //S1输出值到rax，S2不使用rax
#define StoS_DEST_SRC   "atsgx.s2s.dest.src"  //S1输出值到rax，S2使用rax
#define StoS_None_None  "atsgx.s2s.none.none" //S1不输出值到rax，S2不使用rax
#define StoS_None_SRC   "atsgx.s2s.none.src"  //S1不输出值到rax，S2使用rax
//S1->N
#define StoN_DEST  "atsgx.s2n.dest" //S输出值到rax
#define StoN_NONE  "atsgx.s2n.none" //S不输出值到rax
//N->N nothing

namespace llvm{
  template <class Container>
  void split_string(const std::string& str, Container& cont, char delim = ';')
  {
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
      cont.push_back(token);
    }
  }

  enum LeakyState{
    ATSGX_SAFE,  //0
    ATSGX_CODE_LEAKY,  //1
    ATSGX_DATA_LEAKY,  //2
    ATSGX_CODE_DATA_LEAKY,  //3
    ATSGX_UNSAFE,    //4
  };

  template<class Class>
  unsigned ATSGX_isLeaky(Class *object){
    //0 untainted
    //1 code
    //2 data
    //3 code&data
    unsigned state=0;

    if(!object) return state;

    //function and instruction
    if(!object->hasMetadata()) return state;

    MDNode *node = object->getMetadata(ATSGX_CODE_STATE);
    if (node){
      state|=1;
      StringRef strRef =  cast<MDString>(node->getOperand(0))->getString();
      if( strRef == ATSGX_LEAKAGE_STR)
        state |=  ATSGX_CODE_LEAKY;
      else if( strRef == ATSGX_UNSAFE_STR)
        state |= ATSGX_UNSAFE;
    }

    node = object->getMetadata(ATSGX_DATA_STATE);
    if (node && (cast<MDString>(node->getOperand(0))->getString() == ATSGX_LEAKAGE_STR)){
      state |= ATSGX_DATA_LEAKY;
    }

    return state;
  }

  template<class Class>
  unsigned ATSGX_isLeaky(const Class *object){
    //0 untainted
    //1 code
    //2 data
    //3 code&data
    unsigned state=ATSGX_SAFE;

    if(!object) return state;

    //function and instruction
    if(!object->hasMetadata()) return state;

    MDNode *node = object->getMetadata(ATSGX_CODE_STATE);
    if (node){
      StringRef strRef = cast<MDString>(node->getOperand(0))->getString();
      if( strRef == ATSGX_LEAKAGE_STR)
        state |=  ATSGX_CODE_LEAKY;
      else if( strRef == ATSGX_UNSAFE_STR)
        state |= ATSGX_UNSAFE;
    }

    node = object->getMetadata(ATSGX_DATA_STATE);
    if (node && (cast<MDString>(node->getOperand(0))->getString() == ATSGX_LEAKAGE_STR)){
      state |= ATSGX_DATA_LEAKY;
    }

    return state;
  }

//  unsigned ATSGX_MBB_isLeaky(BasicBlock *object){
//    unsigned state=0;
//
//    for(auto &I:*object){
//      state |= ATSGX_isLeaky<Instruction>(&I);
//    }
//    return state;
//  }

  unsigned ATSGX_MBB_isLeaky(const MachineBasicBlock *object);

  template<class Class> void ATSGX_setLeakageState(LLVMContext &ctx, Class *object,
                                                   unsigned state){

    if(state == ATSGX_SAFE) return;

    if(state & ATSGX_DATA_LEAKY){
      MDNode *node = MDNode::get(ctx, MDString::get(ctx, ATSGX_LEAKAGE_STR));
      object->setMetadata(ATSGX_DATA_STATE, node);
    }

    if(state & ATSGX_CODE_LEAKY){
      MDNode *node = MDNode::get(ctx, MDString::get(ctx, ATSGX_LEAKAGE_STR));
      object->setMetadata(ATSGX_CODE_STATE, node);
    }

    if(state & ATSGX_UNSAFE){
      MDNode *node = MDNode::get(ctx, MDString::get(ctx, ATSGX_UNSAFE_STR));
      object->setMetadata(ATSGX_CODE_STATE, node);
    }

    return;
  }

  template<class Class> void ATSGX_setLeakageState(LLVMContext &ctx, const Class *object,
      unsigned state){

    if(state == ATSGX_SAFE) return;

    if(state & ATSGX_DATA_LEAKY){
      MDNode *node = MDNode::get(ctx, MDString::get(ctx, ATSGX_LEAKAGE_STR));
      object->setMetadata(ATSGX_DATA_STATE, node);
    }

    if(state & ATSGX_CODE_LEAKY){
      MDNode *node = MDNode::get(ctx, MDString::get(ctx, ATSGX_LEAKAGE_STR));
      object->setMetadata(ATSGX_CODE_STATE, node);
    }

    if(state & ATSGX_UNSAFE){
      MDNode *node = MDNode::get(ctx, MDString::get(ctx, ATSGX_UNSAFE_STR));
      object->setMetadata(ATSGX_CODE_STATE, node);
    }

    return;
  }

  struct ATSGXIRTagPass : public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    ATSGXIRTagPass(const StringRef &strRef, int o_level, bool ATSGXDebug) : ModulePass(ID),
                                              leakage_file_path(strRef.str()),
                                              ATSGX_Debug(ATSGXDebug), opt_level(o_level){
      initializeATSGXIRTagPassPass(*PassRegistry::getPassRegistry());
    }

    ATSGXIRTagPass(): ModulePass(ID),testing(true), leakage_file_path(" "), ATSGX_Debug(false), opt_level(0){
      initializeATSGXIRTagPassPass(*PassRegistry::getPassRegistry());
    }

    bool testing = false;
    const std::string leakage_file_path;
    bool ATSGX_Debug;
    int  opt_level;

    //filename to absolute path
    typedef std::map<const std::string, const std::string> Str2str;
    typedef std::map<const std::string, std::set<unsigned>> Str2set;
    typedef std::vector<std::string> StringVector;

    Str2str file_2_path_mapping;
    //dirty lines in map
    Str2set dirty_lines;
    Str2set dirty_lines_data;
    Str2str func_2_file_mapping;

    static bool parse_file(const StringRef &file_name, Str2str &file_mapping, Str2str &func_mapping,
        Str2set &dirty_lines, bool debug = false){
      auto file = MemoryBuffer::getFile(file_name);
      if(debug) outs() << file_name <<"\n";
      if (std::error_code EC = file.getError()) {
        errs() << "Could not open the file:"<< file_name <<"\n";
        return false;
      }

      StringVector _temp_splited_strings;
      line_iterator L(*file.get());
      while (!L.is_at_end() ) {
        if(debug) outs() << (*L).str() << "\n";
        _temp_splited_strings.clear();
        split_string<StringVector>((*L).str(), _temp_splited_strings);

        auto & func_name = _temp_splited_strings[0];
        auto & source_file_name = _temp_splited_strings[1];
        auto & line      = _temp_splited_strings[2];
        auto & file_path = _temp_splited_strings[3];

        //file:file_path
        if(file_mapping.find(source_file_name) == file_mapping.end()){
          file_mapping.insert(std::pair<const std::string, const std::string>(
              source_file_name, file_path
          ));
        }

        assert( file_path.compare(file_mapping[source_file_name])==0
                && "The file mapping is not unique");

        //file:<dirty_lines>
        unsigned line_num = (unsigned)std::stoul(line);
        dirty_lines[file_path].insert(line_num);

        //func:file_path
        if(func_mapping.find(func_name) == func_mapping.end()){
          func_mapping.insert(std::pair<const std::string, const std::string>(
              func_name, source_file_name
          ));
        }
        assert( source_file_name.compare(func_mapping[func_name])==0
                && "The func mapping is not unique");

        ++L;
      }
      return true;
    }
    void getAnalysisUsage(AnalysisUsage &AU) const override;
    bool runOnModule(Module &M) override;

    void debug_taint_set(){
      errs()<<"=========="<<__func__<<"============\n";
      errs()<<"list dirty_lines for code\n";
      for(auto itr: dirty_lines){
        const std::string & file_path = itr.first;
        std::set<unsigned> & file_dirty_lines = itr.second;
        errs()<<file_path<<":";
        for(int i:file_dirty_lines){
          errs()<<Twine(i)<<",";
        }
        errs()<<"\n";
      }

      errs()<<"list dirty_lines for data\n";
      for(auto itr: dirty_lines_data){
        const std::string & file_path = itr.first;
        std::set<unsigned> & file_dirty_lines = itr.second;
        errs()<<file_path<<":";
        for(int i:file_dirty_lines){
          errs()<<Twine(i)<<",";
        }
        errs()<<"\n";
      }
    }

    void debug_check_taint_state(Module &M){

      for (auto &F : M) {
        if(ATSGX_SAFE == ATSGX_isLeaky<Function>(&F)){
          continue;
        }
        if(ATSGX_UNSAFE == ATSGX_isLeaky<Function>(&F)){
          errs()<<"Current unsafe func:"<<F.getName()<<"\n";
          continue;
        }
        for (auto &B: F.getBasicBlockList()){
          for(auto &I: B.instructionsWithoutDebug()){
            //get each instruction's debug information
            const DebugLoc &debugLoc = I.getDebugLoc();
            if (!debugLoc.get())
              continue;

            //get the dirty set
            StringRef path = debugLoc->getDirectory();
            StringRef name = debugLoc->getFilename();
            std::string file_path =  path.str()+ "/" + name.str();
            unsigned line = debugLoc.getLine();

            //code
            auto itr_code = dirty_lines.find(file_path);
            if(itr_code != dirty_lines.end()){
              std::set<unsigned > &file_dirty_lines = itr_code->second;

              //match each dirty line
              if(file_dirty_lines.find(line)!=file_dirty_lines.end()){
                continue;
              }
            }

            //data
            auto itr_data = dirty_lines_data.find(file_path);
            if(itr_data != dirty_lines_data.end()){
              std::set<unsigned > &file_dirty_lines = itr_data->second;

              //match each dirty line
              if(file_dirty_lines.find(line)!=file_dirty_lines.end()){
                continue;
              }
            }

          }
        }
      }
      return;
    }

    void debug_show_unsafe_functions(std::set<Function *> working_set){
      for(auto &f:working_set){
        errs()<<"unsafe function << "<< f->getName()<<"\n";
      }
      return;
    }

  };
}

#endif //LLVM_ATSGXIRTAG_H
