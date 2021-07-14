#ifndef LLVM_LIB_TARGET_X86_X86CACHEANALYSIS_H
#define LLVM_LIB_TARGET_X86_X86CACHEANALYSIS_H

#include <vector>
#include <map>
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"

namespace llvm {

  class X86CacheAnalysis {
  public:
    X86CacheAnalysis();
    void doInitialize(MachineFunction &MF, AsmPrinter &AP);
    void doAnalysis();
    void set_count(int num);
    void print_count();

    bool isRAXSrc(unsigned bb_num);
    bool isRAXDst(unsigned bb_num);
    bool isRAXSrcAfterCall(unsigned bb_num, int index);
    bool isRAXDstAfterCall(unsigned bb_num, int index);

    int getBBCacheWayUsage(int bb_num);
    int getJointBBCacheWayUsage(int bb_num1, int bb_num2);
    int getFunCacheWayUsage();
    enum RendezvousState{
      UNCHANGED,
      NEW_ADDED,
      UPDATED,
      REMOVED,
    };

    //ATSGX
    struct Rendezvous{
      int index;
      float threshold = 7.0;
      float penalty_coef = 1.1;

      //entry
      const MachineBasicBlock * rendezvous_entry;

      //basic blocks
      std::set<const MachineBasicBlock*> basic_blocks;

      //record the live out of each block
      std::map<const MachineBasicBlock*, float> blk_cache_num_live_out;

    public:
      Rendezvous(const MachineBasicBlock * entry, int index):rendezvous_entry(entry),index(index){}

      RendezvousState update(const MachineBasicBlock * MBB, float live_out_value){
        RendezvousState state = UNCHANGED;

        //update blk_cache_num_live_out
        auto itr = basic_blocks.find(MBB);
        if(itr==basic_blocks.end()){
          //不存在，需要创建
          blk_cache_num_live_out.insert(std::make_pair(MBB, live_out_value));
          //add basic_blocks
          basic_blocks.emplace(MBB);
          state = NEW_ADDED;
          errs()<<"new: MBB_"<<MBB->getNumber()<<" value:"<< live_out_value<< "\n";
        }else{
          //存在，并且live_out 的值变了
          if(blk_cache_num_live_out[MBB] != live_out_value){
            errs()<<"update: MBB_"<< MBB->getNumber()<<" value:"<< blk_cache_num_live_out[MBB] << "-->" << live_out_value << "\n";
            blk_cache_num_live_out[MBB] = live_out_value;
            state = UPDATED;
          }
        }

        return state;
      }

      RendezvousState remove_blk(const MachineBasicBlock *MBB){
        //remove blk_cache_num_live_out
        auto itr = basic_blocks.find(MBB);
        if(itr==basic_blocks.end()){
          //不存在, 无需删除
          return UNCHANGED;
        }

        //remove from basic_blocks
        basic_blocks.erase(MBB);
        //remove from blk_cache_num_live_out
        blk_cache_num_live_out.erase(MBB);
        return REMOVED;
      }

      bool get_blk_live_out(const MachineBasicBlock *MBB, float &live_out_value){
        live_out_value = 0.0;
        auto _prev_itr = blk_cache_num_live_out.find(MBB);
        if(_prev_itr != blk_cache_num_live_out.end()){
          live_out_value = _prev_itr->second;
          return true;
        }
        return false;
      }

      bool is_mbb_existed(const MachineBasicBlock *MBB){
        auto itr = basic_blocks.find(MBB);
        if(itr!=basic_blocks.end())
          return true;
        else
          return false;
      }

      //is there at least one successor in this Rendezvous
      bool is_subcessor_existed(const MachineBasicBlock *MBB){
        for(const MachineBasicBlock* succ: MBB->successors()){
          auto itr = basic_blocks.find(succ);
          if(itr!=basic_blocks.end()){
            return true;
          }
        }
        return false;
      }

      void clear_invalid_blks();
      int get_index(){
        return index;
      }
    };
    void analyzeCacheLiveOut();
    void analyzeMBBCacheLiveOut(const MachineBasicBlock *BB);
    void _analyzeMBBCacheLiveOut(Rendezvous * rendezvous, const MachineBasicBlock *BB,
        std::set<const MachineBasicBlock*> &working_set);
    bool checkAllInLoop(Rendezvous *rendezvous, const MachineLoop * ML);

    Rendezvous * getRendezvous(const MachineBasicBlock* MBB){
      //if MBB in one rendezvous, return it, otherwise nullptr;
      auto itr = revert_live_out_info.find(MBB);
      if(itr!=revert_live_out_info.end()){
        return itr->second;
      }
      return nullptr;
    }

  private:
    const AsmPrinter *AP;
    const MachineFunction *MF;
    const MachineLoopInfo *LI;

    // tarjan algorithm
    int num;
    std::vector<const MachineBasicBlock*> stack;
    std::vector<const MachineBasicBlock*> scc_stack;
    std::vector<const MachineBasicBlock*> sort_stack;
    std::map<const MachineBasicBlock*, int> order;
    std::map<const MachineBasicBlock*, int> link;
    void tarjan_init();
    void tarjan(const MachineBasicBlock *MB);

    //ATSGX
    //record all rendezvous
    //note: the first bb in each rendezvous is the corresponding "entry bb"
    std::vector<Rendezvous *> blk_live_out_recoder;
    //record which rendezvous this bb belongs to;
    std::map<const MachineBasicBlock*, Rendezvous *> revert_live_out_info;

    // RAX access pattern analysis
    typedef struct {
      bool is_src;
      bool is_dst;
    } access_pattern;

    typedef struct {
      access_pattern bb_pattern;
      std::map<int, access_pattern> call_pattern;
    } rax_access_pattern;

    std::map<unsigned, rax_access_pattern> rax_analysis_result;

    void doRAXPatternAnalysis(const MachineBasicBlock *MB);

    // Cache access analysis

    // Instruction Classification
    typedef enum {
      MEM_IMM,
      MEM_MEM,
      MEM_REG
    } Mem_Type;

    typedef enum {
      Op_MOV,
      Op_LEA,
      Op_MUL,
      Op_IMUL,
      Op_DIV,
      Op_IDIV,
      Op_NEG,
      Op_NOT,
      Op_INC,
      Op_DEC,
      Op_AND,
      Op_OR,
      Op_XOR,
      Op_ADD,
      Op_SUB,
      Op_ADC,
      Op_SBB,
      Op_CMP,
      Op_TEST,
      Op_POP,
      Op_PUSH
    } Instr_Operation;

    typedef enum {
      Ty_I,
      Ty_R,
      Ty_M,
      Ty_RR,
      Ty_RI,
      Ty_RM,
      Ty_MR,
      Ty_MI,
      Ty_RR_REV,
      Ty_RRI,
      Ty_RMI,
    } Instr_Operator_Type;

    void simulateInstruction(const MachineInstr *MI, Instr_Operation OP, Instr_Operator_Type Ty);
    void processInstruction(const MachineInstr *MI);
    void processBaicBlock(const MachineBasicBlock *MB);

    // Value-Set Analysis (VSA)
    typedef enum {
      RGN_GLOBAL,
      RGN_LOCAL,
      RGN_HEAP,
      RGN_NONE
    } VSA_Memory_Region;

    typedef struct {
      unsigned int stride;
      int lower_bound;
      int upper_bound;
      bool has_value;
    } VSA_Strided_Interval;

    typedef struct vsa_alocs{
      VSA_Memory_Region rgn;
      VSA_Strided_Interval values;
      VSA_Strided_Interval addresses;
      int id;
      int src_id;
      struct vsa_alocs *src;
    } VSA_A_Locs;

    typedef struct {
      VSA_A_Locs *alocs;
      int64_t value;
      bool isALocs;
    } ValueOrALocs;

    std::vector<const GlobalValue *> global_value_list;
    std::map<unsigned, VSA_A_Locs> register_alocs;
    std::map<std::string, VSA_A_Locs> global_alocs;
    std::map<int, VSA_A_Locs> local_alocs;

    int alocs_id;

    void VSA_init();
    void init_alocs(VSA_A_Locs *alocs, VSA_Memory_Region rgn);
    void set_alocs(VSA_A_Locs **alocs, const MachineInstr *MI, Mem_Type Ty, int index);

    void mov_alocs(ValueOrALocs *src, VSA_A_Locs *dst);
    void lea_alocs(ValueOrALocs *src, VSA_A_Locs *dst);
    void add_alocs(ValueOrALocs *src, VSA_A_Locs *dst);
    void cmp_alocs(ValueOrALocs *src, VSA_A_Locs *dst);

    std::map<int, std::map<int, std::vector<bool>>> cache_model;
    void classifyMemoryWrite(const MachineInstr *MI, int index);
    void cache_write(int bb_num, int id, int offset);

    // Debug
    typedef enum {
      DBG_DST,
      DBG_SRC
    } Debug_Target_Type;

    void debug_all_operators(const MachineInstr*);
    void debug_reg_alisas(unsigned);
    void debug_operators(const MachineInstr *, Debug_Target_Type, Mem_Type, int);
    void debug_alocs(VSA_A_Locs *alocs, const void *index);
    void debug_cache_access(int bb_num);
    void debug_fun_cache_access();

    int count;
  };
}

#endif
